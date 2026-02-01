#include "text_extractor.h"
#include "logger.h"
#include <gumbo.h>
#include <regex>
#include <algorithm>
#include <sstream>
#include <cctype>

TextExtractor::TextExtractor() {
    // Default selectors to remove
    set_remove_selectors("nav, footer, script, style, noscript, svg, "
        "img[src^='data:'], "
        "[role=\"alert\"], "
        "[role=\"banner\"], "
        "[role=\"dialog\"], "
        "[role=\"alertdialog\"], "
        "[role=\"region\"][aria-label*=\"skip\" i], "
        "[aria-modal=\"true\"]");
}

TextExtractor::~TextExtractor() {
}

void TextExtractor::set_remove_selectors(const std::string& selectors) {
    remove_selectors_.clear();
    
    // Parse comma-separated selectors
    std::istringstream iss(selectors);
    std::string selector;
    
    while (std::getline(iss, selector, ',')) {
        // Trim whitespace
        selector.erase(0, selector.find_first_not_of(" \t\n\r"));
        selector.erase(selector.find_last_not_of(" \t\n\r") + 1);
        
        if (!selector.empty()) {
            remove_selectors_.push_back(selector);
        }
    }
}

TextExtraction TextExtractor::extract_from_html(const std::string& html, const std::string& url) {
    TextExtraction result;
    
    // Parse HTML with Gumbo
    GumboOutput* output = gumbo_parse(html.c_str());
    if (!output) {
        Logger::instance().error("TextExtractor: Failed to parse HTML");
        return result;
    }
    
    // Extract title
    GumboNode* title_node = nullptr;
    GumboVector* children = &output->root->v.element.children;
    
    for (unsigned int i = 0; i < children->length; i++) {
        GumboNode* node = static_cast<GumboNode*>(children->data[i]);
        if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_HEAD) {
            GumboVector* head_children = &node->v.element.children;
            for (unsigned int j = 0; j < head_children->length; j++) {
                GumboNode* head_child = static_cast<GumboNode*>(head_children->data[j]);
                if (head_child->type == GUMBO_NODE_ELEMENT && 
                    head_child->v.element.tag == GUMBO_TAG_TITLE) {
                    title_node = head_child;
                    break;
                }
            }
            break;
        }
    }
    
    if (title_node && title_node->v.element.children.length > 0) {
        GumboNode* text_node = static_cast<GumboNode*>(title_node->v.element.children.data[0]);
        if (text_node->type == GUMBO_NODE_TEXT) {
            result.title = text_node->v.text.text;
        }
    }
    
    // Extract body text
    GumboNode* body_node = nullptr;
    for (unsigned int i = 0; i < children->length; i++) {
        GumboNode* node = static_cast<GumboNode*>(children->data[i]);
        if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_BODY) {
            body_node = node;
            break;
        }
    }
    
    if (body_node) {
        bool in_code = false;
        result.text = extract_text_recursive(body_node, in_code);
        result.plain_text = result.text;
        
        // Remove markdown formatting from plain text
        std::regex md_regex(R"(#+\s|```[\s\S]*?```|\*\*|__|\*|_|`|>|-)");
        result.plain_text = std::regex_replace(result.plain_text, md_regex, "");
        
        // Clean up extra whitespace
        result.plain_text = normalize_text(result.plain_text);
        
        // Extract code blocks
        extract_code_blocks(body_node, result.code_blocks);
    }
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    Logger::instance().info("TextExtractor: Extracted text from HTML: " + 
                 std::to_string(result.text.length()) + " chars");
    
    return result;
}

std::string TextExtractor::extract_text_recursive(void* node_ptr, bool& in_code) {
    if (!node_ptr) return "";
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    std::string result;
    
    if (node->type == GUMBO_NODE_TEXT) {
        std::string text = node->v.text.text;
        text = normalize_text(text);
        return text;
    }
    
    if (node->type != GUMBO_NODE_ELEMENT) {
        return "";
    }
    
    // Check if element should be removed
    if (should_remove_element(node)) {
        return "";
    }
    
    // Handle specific tags
    GumboTag tag = node->v.element.tag;
    
    // Heading tags -> markdown
    if (tag == GUMBO_TAG_H1) {
        result += "# ";
    } else if (tag == GUMBO_TAG_H2) {
        result += "## ";
    } else if (tag == GUMBO_TAG_H3) {
        result += "### ";
    } else if (tag == GUMBO_TAG_H4) {
        result += "#### ";
    } else if (tag == GUMBO_TAG_H5) {
        result += "##### ";
    } else if (tag == GUMBO_TAG_H6) {
        result += "###### ";
    }
    
    // Code blocks
    if (tag == GUMBO_TAG_PRE || tag == GUMBO_TAG_CODE) {
        in_code = true;
    }
    
    // Process children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++) {
        std::string child_text = extract_text_recursive(children->data[i], in_code);
        result += child_text;
    }
    
    if (tag == GUMBO_TAG_PRE || tag == GUMBO_TAG_CODE) {
        in_code = false;
        
        // Try to detect language
        std::string language = detect_language(result);
        result = "```" + language + "\n" + result + "\n```\n";
    }
    
    // Add line breaks for block elements
    if (tag == GUMBO_TAG_P || tag == GUMBO_TAG_DIV || 
        tag == GUMBO_TAG_H1 || tag == GUMBO_TAG_H2 || tag == GUMBO_TAG_H3 ||
        tag == GUMBO_TAG_H4 || tag == GUMBO_TAG_H5 || tag == GUMBO_TAG_H6 ||
        tag == GUMBO_TAG_LI || tag == GUMBO_TAG_BLOCKQUOTE) {
        result += "\n";
    }
    
    // Lists
    if (tag == GUMBO_TAG_LI) {
        result = "- " + result;
    }
    
    // Bold/Strong
    if (tag == GUMBO_TAG_STRONG || tag == GUMBO_TAG_B) {
        result = "**" + result + "**";
    }
    
    // Italic/Em
    if (tag == GUMBO_TAG_EM || tag == GUMBO_TAG_I) {
        result = "*" + result + "*";
    }
    
    // Inline code
    if (tag == GUMBO_TAG_CODE && !in_code) {
        result = "`" + result + "`";
    }
    
    // Links
    if (tag == GUMBO_TAG_A) {
        GumboAttribute* href_attr = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href_attr) {
            result = "[" + result + "](" + href_attr->value + ")";
        }
    }
    
    // Blockquotes
    if (tag == GUMBO_TAG_BLOCKQUOTE) {
        // Add quote prefix to each line
        std::istringstream iss(result);
        std::string line;
        std::string quoted;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                quoted += "> " + line + "\n";
            }
        }
        result = quoted;
    }
    
    return result;
}

std::string TextExtractor::detect_language(const std::string& code_snippet) {
    std::string trimmed = code_snippet;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    // Check for common language indicators
    std::vector<std::pair<std::string, std::string>> patterns = {
        {"function\\s+\\w+\\s*\\(", "js"},
        {"const\\s+\\w+\\s*=", "js"},
        {"let\\s+\\w+\\s*=", "js"},
        {"import\\s+\\{", "js"},
        {"<?php", "php"},
        {"class\\s+\\w+\\s*:", "python"},
        {"def\\s+\\w+\\s*\\(", "python"},
        {"import\\s+\\w+", "python"},
        {"#include\\s*<", "cpp"},
        {"#include\\s+\"", "cpp"},
        {"pub\\s+fn\\s+", "rust"},
        {"fn\\s+\\w+\\s*\\(", "rust"},
        {"func\\s+\\w+\\s*\\(", "go"},
        {"package\\s+", "go"},
        {"struct\\s+\\w+\\s*\\{", "go"},
        {"CREATE\\s+TABLE", "sql"},
        {"SELECT\\s+\\*?\\s+FROM", "sql"},
        {"<html>|<!DOCTYPE", "html"},
        {"\\.css|@media", "css"},
        {"\\$\\(\\w+\\)|#!/bin/bash", "bash"},
    };
    
    for (const auto& [pattern, lang] : patterns) {
        std::regex re(pattern, std::regex::icase);
        if (std::regex_search(trimmed, re)) {
            return lang;
        }
    }
    
    return "";  // No language detected
}

std::string TextExtractor::normalize_text(const std::string& text) {
    std::string result = text;
    
    // Replace multiple spaces with single space
    std::regex space_regex("  +");
    result = std::regex_replace(result, space_regex, " ");
    
    // Replace multiple newlines with double newline
    std::regex newline_regex("\\n{3,}");
    result = std::regex_replace(result, newline_regex, "\n\n");
    
    // Trim leading/trailing whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

bool TextExtractor::should_remove_element(void* node_ptr) {
    if (!node_ptr) return false;
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    
    // Always remove script and style tags
    if (node->v.element.tag == GUMBO_TAG_SCRIPT || 
        node->v.element.tag == GUMBO_TAG_STYLE ||
        node->v.element.tag == GUMBO_TAG_NOSCRIPT ||
        node->v.element.tag == GUMBO_TAG_NAV ||
        node->v.element.tag == GUMBO_TAG_FOOTER) {
        return true;
    }
    
    // Check role attribute
    GumboAttribute* role_attr = gumbo_get_attribute(&node->v.element.attributes, "role");
    if (role_attr) {
        std::string role = role_attr->value;
        if (role == "alert" || role == "banner" || role == "dialog" || 
            role == "alertdialog" || role == "presentation" || role == "none") {
            return true;
        }
    }
    
    // Check aria-modal attribute
    GumboAttribute* modal_attr = gumbo_get_attribute(&node->v.element.attributes, "aria-modal");
    if (modal_attr && std::string(modal_attr->value) == "true") {
        return true;
    }
    
    // Check for skip regions
    GumboAttribute* aria_label = gumbo_get_attribute(&node->v.element.attributes, "aria-label");
    if (aria_label && std::string(aria_label->value).find("skip") != std::string::npos) {
        return true;
    }
    
    return false;
}

std::string TextExtractor::get_element_tag(void* node_ptr) {
    if (!node_ptr) return "";
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    if (node->type != GUMBO_NODE_ELEMENT) return "";
    
    return gumbo_normalized_tagname(node->v.element.tag);
}

std::string TextExtractor::get_element_text(void* node_ptr) {
    if (!node_ptr) return "";
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    
    if (node->type == GUMBO_NODE_TEXT) {
        return node->v.text.text;
    }
    
    std::string result;
    GumboVector* children = &node->v.element.children;
    
    for (unsigned int i = 0; i < children->length; i++) {
        result += get_element_text(children->data[i]);
    }
    
    return result;
}

bool TextExtractor::is_code_element(void* node_ptr) {
    if (!node_ptr) return false;
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    if (node->type != GUMBO_NODE_ELEMENT) return false;
    
    return node->v.element.tag == GUMBO_TAG_CODE || 
           node->v.element.tag == GUMBO_TAG_PRE;
}

void TextExtractor::extract_code_blocks(void* node_ptr, std::vector<std::string>& code_blocks) {
    if (!node_ptr) return;
    
    GumboNode* node = static_cast<GumboNode*>(node_ptr);
    
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboTag tag = node->v.element.tag;
        
        // Found code block
        if (tag == GUMBO_TAG_PRE || tag == GUMBO_TAG_CODE) {
            std::string code_text = get_element_text(node);
            code_text = normalize_text(code_text);
            
            if (!code_text.empty()) {
                std::string language = detect_language(code_text);
                std::string code_with_lang = "```" + language + "\n" + code_text + "\n```";
                code_blocks.push_back(code_with_lang);
            }
        }
        
        // Recursively process children
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; i++) {
            extract_code_blocks(children->data[i], code_blocks);
        }
    }
}
