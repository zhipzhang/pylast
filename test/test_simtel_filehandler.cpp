#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#define DEBUG
#include "SimtelFileHandler.hh"
#include <filesystem>
#include <memory> // Required for std::unique_ptr


class SimtelFileHandlerTestSuite
{
    public:
        SimtelFileHandlerTestSuite() = default;
        ~SimtelFileHandlerTestSuite() = default;

        std::unique_ptr<SimtelFileHandler> simtel_file_handler;
};

TEST_CASE_FIXTURE(SimtelFileHandlerTestSuite, "OpenFile")
{
    SUBCASE("Non-Exist file")
    {
        CHECK_THROWS_AS(SimtelFileHandler("non_exist.simtel.gz"), std::runtime_error);
    }
    SUBCASE("Open Test file")
    {
        auto test_directory = std::filesystem::path(__FILE__);
        auto test_file = test_directory.parent_path() / "test_data" / "simtel.zst";
        simtel_file_handler = std::make_unique<SimtelFileHandler>(test_file.string());
        CHECK(simtel_file_handler != nullptr);
    }
};

TEST_CASE_FIXTURE(SimtelFileHandlerTestSuite, "MethodTest")
{
    auto test_directory = std::filesystem::path(__FILE__);
    auto test_file = test_directory.parent_path() / "test_data" / "simtel.zst";
    simtel_file_handler = std::make_unique<SimtelFileHandler>(test_file.string());
    REQUIRE(simtel_file_handler != nullptr);
    simtel_file_handler->read_until_event(); 

    SUBCASE("read_until_block")
    {
        simtel_file_handler->read_until_block(SimtelFileHandler::BlockType::Mc_Event);
        CHECK(simtel_file_handler->item_header.type == static_cast<unsigned long>( SimtelFileHandler::BlockType::Mc_Event));
    }
    SUBCASE("only_read_blocks")
    {
        simtel_file_handler->only_read_blocks({SimtelFileHandler::BlockType::Mc_Event, SimtelFileHandler::BlockType::Mc_Shower});
        CHECK(simtel_file_handler->item_header.type == static_cast<unsigned long>( SimtelFileHandler::BlockType::Mc_Event));
    }
    SUBCASE("only_read_block_nonexist")
    {
        simtel_file_handler->only_read_blocks({SimtelFileHandler::BlockType::TEST_BLOCK, SimtelFileHandler::BlockType::Mc_Event, SimtelFileHandler::BlockType::Mc_Shower});
        CHECK(simtel_file_handler->no_more_blocks == true);
    }

}