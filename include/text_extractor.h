#ifndef TEXT_EXTRACTOR_H
#define TEXT_EXTRACTOR_H

#include <string>
#include <vector>

struct TextExtraction {
    std::string title;
    std::string text;           // Markdown formatted text
    std::string plain_text;     // Plain text without markdown
    std::vector<std::string> code_blocks;  // Code blocks with language
    std::vector<std::string> links;
};

class TextExtractor {
public:
    TextExtractor();
    ~TextExtractor();
    
    /**
     * Extract text from HTML using Gumbo parser
     * @param html HTML content
     * @param url Original URL (for relative link resolution)
     * @return TextExtraction structure with extracted content
     */
    TextExtraction extract_from_html(const std::string& html, const std::string& url);
    
    /**
     * Set CSS selectors for elements to remove
     * @param selectors Comma-separated CSS selectors
     */
    void set_remove_selectors(const std::string& selectors);
    
private:
    std::vector<std::string> remove_selectors_;
    
    // Helper methods
    std::string extract_text_recursive(void* node, bool& in_code);
    std::string detect_language(const std::string& code_snippet);
    std::string normalize_text(const std::string& text);
    bool should_remove_element(void* node);
    std::string get_element_tag(void* node);
    std::string get_element_text(void* node);
    bool is_code_element(void* node);
    void extract_code_blocks(void* node, std::vector<std::string>& code_blocks);
};

#endif // TEXT_EXTRACTOR_H
