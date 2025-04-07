#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/args.hpp"
#include "coil/coil.hpp"
#include <string>
#include <memory>
#include <vector>

using namespace coil;
using namespace testing;

class ArgParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize COIL
        ASSERT_TRUE(initialize());
        
        // Create a test argument parser
        parser = ArgParser::create(
            "test_program",
            "Test program description",
            "Test program epilog"
        );
        ASSERT_NE(parser, nullptr);
    }
    
    void TearDown() override {
        parser.reset();
        cleanup();
    }
    
    std::shared_ptr<ArgParser> parser;
    
    // Helper to create argc/argv
    std::vector<char*> createArgv(const std::vector<std::string>& args) {
        std::vector<char*> argv;
        
        // Add program name
        argv.push_back(const_cast<char*>("test_program"));
        
        // Add arguments
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        
        return argv;
    }
};

TEST_F(ArgParserTest, AddFlag) {
    // Add a flag
    EXPECT_TRUE(parser->addFlag('f', "flag", "Test flag", false));
    
    // Parse arguments
    std::vector<std::string> args = {"--flag"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify flag was set
    EXPECT_TRUE(parser->getFlag("flag"));
    
    // Verify short name
    args = {"-f"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_TRUE(parser->getFlag("flag"));
}

TEST_F(ArgParserTest, AddString) {
    // Add a string argument
    EXPECT_TRUE(parser->addString('s', "string", "Test string", "STRING", "default", false));
    
    // Parse arguments
    std::vector<std::string> args = {"--string", "value"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify string was set
    EXPECT_EQ(parser->getString("string"), "value");
    
    // Verify short name
    args = {"-s", "short"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_EQ(parser->getString("string"), "short");
    
    // Verify default value
    args = {};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_EQ(parser->getString("string"), "default");
}

TEST_F(ArgParserTest, AddInt) {
    // Add an int argument
    EXPECT_TRUE(parser->addInt('i', "int", "Test int", "INT", 42, false));
    
    // Parse arguments
    std::vector<std::string> args = {"--int", "123"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify int was set
    EXPECT_EQ(parser->getInt("int"), 123);
    
    // Verify short name
    args = {"-i", "456"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_EQ(parser->getInt("int"), 456);
    
    // Verify default value
    args = {};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_EQ(parser->getInt("int"), 42);
    
    // Verify invalid int
    args = {"--int", "not_an_int"};
    argv = createArgv(args);
    EXPECT_FALSE(parser->parse(argv.size(), argv.data()));
}

TEST_F(ArgParserTest, AddFloat) {
    // Add a float argument
    EXPECT_TRUE(parser->addFloat('f', "float", "Test float", "FLOAT", 3.14f, false));
    
    // Parse arguments
    std::vector<std::string> args = {"--float", "2.718"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify float was set
    EXPECT_FLOAT_EQ(parser->getFloat("float"), 2.718f);
    
    // Verify short name
    args = {"-f", "1.414"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_FLOAT_EQ(parser->getFloat("float"), 1.414f);
    
    // Verify default value
    args = {};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_FLOAT_EQ(parser->getFloat("float"), 3.14f);
    
    // Verify invalid float
    args = {"--float", "not_a_float"};
    argv = createArgv(args);
    EXPECT_FALSE(parser->parse(argv.size(), argv.data()));
}

TEST_F(ArgParserTest, AddPositional) {
    // Add a positional argument
    EXPECT_TRUE(parser->addPositional("command", "Test command", true));
    
    // Parse arguments
    std::vector<std::string> args = {"run"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify positional argument was set
    size_t count;
    char const* const* positional = parser->getPositional(&count);
    ASSERT_EQ(count, 1);
    EXPECT_STREQ(positional[0], "run");
    
    // Verify required positional
    args = {};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    EXPECT_FALSE(parser->validate());
}

TEST_F(ArgParserTest, MultiplePositional) {
    // Add multiple positional arguments
    EXPECT_TRUE(parser->addPositional("command", "Test command", true));
    EXPECT_TRUE(parser->addPositional("subcommand", "Test subcommand", false));
    
    // Parse arguments
    std::vector<std::string> args = {"run", "test"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify positional arguments were set
    size_t count;
    char const* const* positional = parser->getPositional(&count);
    ASSERT_EQ(count, 2);
    EXPECT_STREQ(positional[0], "run");
    EXPECT_STREQ(positional[1], "test");
    
    // Verify with only one positional
    args = {"run"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    positional = parser->getPositional(&count);
    ASSERT_EQ(count, 1);
    EXPECT_STREQ(positional[0], "run");
    
    // Verify validation passes with only one positional
    EXPECT_TRUE(parser->validate());
}

TEST_F(ArgParserTest, MixedArguments) {
    // Add mixed arguments
    EXPECT_TRUE(parser->addFlag('v', "verbose", "Verbose mode", false));
    EXPECT_TRUE(parser->addString('o', "output", "Output file", "FILE", "out.txt", false));
    EXPECT_TRUE(parser->addInt('n', "num", "Number of iterations", "NUM", 10, false));
    EXPECT_TRUE(parser->addPositional("command", "Command to run", true));
    
    // Parse arguments
    std::vector<std::string> args = {
        "--verbose",
        "-o", "result.txt",
        "--num", "20",
        "run"
    };
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify all arguments were set
    EXPECT_TRUE(parser->getFlag("verbose"));
    EXPECT_EQ(parser->getString("output"), "result.txt");
    EXPECT_EQ(parser->getInt("num"), 20);
    
    size_t count;
    char const* const* positional = parser->getPositional(&count);
    ASSERT_EQ(count, 1);
    EXPECT_STREQ(positional[0], "run");
}

TEST_F(ArgParserTest, WasProvided) {
    // Add arguments
    EXPECT_TRUE(parser->addFlag('v', "verbose", "Verbose mode", false));
    EXPECT_TRUE(parser->addString('o', "output", "Output file", "FILE", "out.txt", false));
    
    // Parse arguments with only verbose
    std::vector<std::string> args = {"--verbose"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify only verbose was provided
    EXPECT_TRUE(parser->wasProvided("verbose"));
    EXPECT_FALSE(parser->wasProvided("output"));
    
    // Parse arguments with both
    args = {"--verbose", "--output", "result.txt"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify both were provided
    EXPECT_TRUE(parser->wasProvided("verbose"));
    EXPECT_TRUE(parser->wasProvided("output"));
}

TEST_F(ArgParserTest, HelpRequested) {
    // Add standard arguments
    parser->addStandardArgs();
    
    // Parse with --help
    std::vector<std::string> args = {"--help"};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify help was requested
    EXPECT_TRUE(parser->helpRequested());
    
    // Parse with -h
    args = {"-h"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify help was requested
    EXPECT_TRUE(parser->helpRequested());
    
    // Parse without help
    args = {"--verbose"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Verify help was not requested
    EXPECT_FALSE(parser->helpRequested());
}

TEST_F(ArgParserTest, RequiredArguments) {
    // Add a required argument
    EXPECT_TRUE(parser->addString('r', "required", "Required argument", "REQ", "", true));
    
    // Parse without the required argument
    std::vector<std::string> args = {};
    auto argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Validate should fail
    EXPECT_FALSE(parser->validate());
    
    // Parse with the required argument
    args = {"--required", "value"};
    argv = createArgv(args);
    EXPECT_TRUE(parser->parse(argv.size(), argv.data()));
    
    // Validate should pass
    EXPECT_TRUE(parser->validate());
}

TEST_F(ArgParserTest, PrintHelp) {
    // Add some arguments
    parser->addStandardArgs();
    parser->addString('o', "output", "Output file", "FILE", "out.txt", false);
    parser->addInt('n', "num", "Number of iterations", "NUM", 10, false);
    parser->addPositional("command", "Command to run", true);
    
    // This just tests that printHelp doesn't crash
    testing::internal::CaptureStdout();
    parser->printHelp();
    std::string output = testing::internal::GetCapturedStdout();
    
    // Verify output contains expected parts
    EXPECT_THAT(output, HasSubstr("Usage:"));
    EXPECT_THAT(output, HasSubstr("test_program"));
    EXPECT_THAT(output, HasSubstr("Test program description"));
    EXPECT_THAT(output, HasSubstr("Options:"));
    EXPECT_THAT(output, HasSubstr("--help"));
    EXPECT_THAT(output, HasSubstr("--verbose"));
    EXPECT_THAT(output, HasSubstr("--output"));
    EXPECT_THAT(output, HasSubstr("--num"));
    EXPECT_THAT(output, HasSubstr("command"));
    EXPECT_THAT(output, HasSubstr("Test program epilog"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}