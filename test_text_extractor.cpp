#include "text_extractor.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== Testing TextExtractor ===" << std::endl;
    
    TextExtractor extractor;
    
    // Test 1: Simple HTML with headings
    std::string html1 = R"(
        <html>
            <head><title>Test Page</title></head>
            <body>
                <h1>Main Title</h1>
                <p>Some paragraph text.</p>
                <h2>Subtitle</h2>
                <p>More content here.</p>
            </body>
        </html>
    )";
    
    TextExtraction result1 = extractor.extract_from_html(html1, "http://test.com");
    std::cout << "✓ Test 1: Headings conversion" << std::endl;
    std::cout << "  Title: " << result1.title << std::endl;
    assert(result1.title == "Test Page");
    
    // Test 2: HTML with code
    std::string html2 = R"(
        <html>
            <body>
                <h1>Code Example</h1>
                <pre><code>function hello() {
    console.log("Hello World");
}</code></pre>
                <p>That was JavaScript code.</p>
            </body>
        </html>
    )";
    
    TextExtraction result2 = extractor.extract_from_html(html2, "http://test.com");
    std::cout << "✓ Test 2: Code block detection" << std::endl;
    assert(!result2.code_blocks.empty());
    
    // Test 3: HTML with formatting
    std::string html3 = R"(
        <html>
            <body>
                <p>This is <strong>bold</strong> and <em>italic</em> text.</p>
                <a href="https://example.com">Link to example</a>
            </body>
        </html>
    )";
    
    TextExtraction result3 = extractor.extract_from_html(html3, "http://test.com");
    std::cout << "✓ Test 3: Text formatting" << std::endl;
    std::cout << "  Formatted: " << result3.text << std::endl;
    
    // Test 4: Elements removal
    std::string html4 = R"(
        <html>
            <body>
                <nav>Navigation</nav>
                <main>Main content</main>
                <footer>Footer</footer>
            </body>
        </html>
    )";
    
    TextExtraction result4 = extractor.extract_from_html(html4, "http://test.com");
    std::cout << "✓ Test 4: Element removal" << std::endl;
    assert(result4.text.find("Navigation") == std::string::npos);
    assert(result4.text.find("Footer") == std::string::npos);
    assert(result4.text.find("Main") != std::string::npos);
    
    std::cout << "\n✓ All TextExtractor tests passed!" << std::endl;
    
    return 0;
}
