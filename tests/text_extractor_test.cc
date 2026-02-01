#include "text_extractor.h"
#include <gtest/gtest.h>

class TextExtractorTest : public ::testing::Test {
protected:
    TextExtractor extractor;
};

TEST_F(TextExtractorTest, HeadingConversion) {
    std::string html = R"(
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
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_EQ(result.title, "Test Page");
    EXPECT_TRUE(result.text.find("#") != std::string::npos);
    EXPECT_TRUE(result.text.find("##") != std::string::npos);
}

TEST_F(TextExtractorTest, CodeBlockDetection) {
    std::string html = R"(
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
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_FALSE(result.code_blocks.empty());
    EXPECT_TRUE(result.text.find("```") != std::string::npos);
}

TEST_F(TextExtractorTest, TextFormatting) {
    std::string html = R"(
        <html>
            <body>
                <p>This is <strong>bold</strong> and <em>italic</em> text.</p>
                <a href="https://example.com">Link to example</a>
            </body>
        </html>
    )";
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_TRUE(result.text.find("**bold**") != std::string::npos);
    EXPECT_TRUE(result.text.find("*italic*") != std::string::npos);
    EXPECT_TRUE(result.text.find("[Link") != std::string::npos);
    EXPECT_TRUE(result.text.find("example.com") != std::string::npos);
}

TEST_F(TextExtractorTest, ElementRemoval) {
    std::string html = R"(
        <html>
            <body>
                <nav>Navigation</nav>
                <main>Main content</main>
                <footer>Footer</footer>
            </body>
        </html>
    )";
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_EQ(result.text.find("Navigation"), std::string::npos);
    EXPECT_EQ(result.text.find("Footer"), std::string::npos);
    EXPECT_TRUE(result.text.find("Main") != std::string::npos);
}

TEST_F(TextExtractorTest, PlainTextExtraction) {
    std::string html = R"(
        <html>
            <body>
                <h1>Title</h1>
                <p>This is <strong>bold</strong> text.</p>
            </body>
        </html>
    )";
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_FALSE(result.plain_text.empty());
    EXPECT_TRUE(result.plain_text.find("bold") != std::string::npos);
}

TEST_F(TextExtractorTest, LanguageDetection) {
    std::string html = R"(
        <html>
            <body>
                <pre><code>const x = 42;
function test() {
    console.log(x);
}</code></pre>
            </body>
        </html>
    )";
    
    TextExtraction result = extractor.extract_from_html(html, "http://test.com");
    
    EXPECT_FALSE(result.code_blocks.empty());
    EXPECT_TRUE(result.code_blocks[0].find("```js") != std::string::npos ||
                result.code_blocks[0].find("```") != std::string::npos);
}
