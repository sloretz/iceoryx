// Copyright (c) 2020 - 2021 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx/tests/posh/moduletests/test_input_path.hpp"
#include "iceoryx_posh/gateway/toml_gateway_config_parser.hpp"
#include "stubs/stub_toml_gateway_config_parser.hpp"
#include "test.hpp"

using namespace ::testing;
using ::testing::_;

using namespace iox::config;

// ======================================== Helpers ======================================== //
namespace
{
using ParseErrorInputFile_t = std::pair<iox::config::TomlGatewayConfigParseError, iox::roudi::ConfigFilePathString_t>;
using CheckCharactersValidity_t = std::pair<std::string, bool>;
}; // namespace

// ======================================== Fixture ======================================== //
class TomlGatewayConfigParserTest : public TestWithParam<ParseErrorInputFile_t>
{
  public:
    void SetUp()
    {
        // get file path via cmake
        m_configFilePath = iox::testing::TEST_INPUT_PATH;
    };
    void TearDown(){};

    iox::roudi::ConfigFilePathString_t m_configFilePath;
};

class TomlGatewayConfigParserSuiteTest : public TestWithParam<CheckCharactersValidity_t>
{
  public:
    void SetUp()
    {
        // get file path via cmake
        m_configFilePath = iox::testing::TEST_INPUT_PATH;
    };
    void TearDown(){};

    iox::roudi::ConfigFilePathString_t m_configFilePath;
    void CreateTmpTomlFile(std::shared_ptr<cpptoml::table> toml)
    {
        m_configFilePath.append(iox::cxx::TruncateToCapacity, "generated_gateway_config.toml");
        std::fstream fs(m_configFilePath, std::fstream::out | std::fstream::trunc);
        if (fs.std::fstream::is_open())
        {
            fs << *toml;
        }
        else
        {
            ASSERT_STREQ("expected open fstream", "fstream not open");
        }
        fs.close();
    }
};

// ======================================== Tests ======================================== //

INSTANTIATE_TEST_SUITE_P(ValidTest,
                         TomlGatewayConfigParserSuiteTest,
                         ::testing::Values(CheckCharactersValidity_t{"validcharacters", false},
                                           CheckCharactersValidity_t{"UPPERCASECHARACTERS", false},
                                           CheckCharactersValidity_t{"lowercasecharacters", false},
                                           CheckCharactersValidity_t{"Number1234567890", false},
                                           CheckCharactersValidity_t{"Under_score_Characters", false},
                                           CheckCharactersValidity_t{"_BeginsWithUnderscore", false},
                                           CheckCharactersValidity_t{"Hyphen-InService", true},
                                           CheckCharactersValidity_t{"1234567890", true},
                                           CheckCharactersValidity_t{"這場考試_!*#:", true}));


TEST_P(TomlGatewayConfigParserSuiteTest, CheckCharactersUsedInServiceDescription)
{
    ::testing::Test::RecordProperty("TEST_ID", "48c13126-f1f9-457f-8e3b-78a27f451101");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntry = cpptoml::make_table();
    const auto charactersValidity = GetParam();

    std::string stringentry = charactersValidity.first;
    serviceEntry->insert("service", stringentry);
    serviceEntry->insert("instance", stringentry);
    serviceEntry->insert("event", stringentry);
    serviceArray->push_back(serviceEntry);
    toml->insert("services", serviceArray);

    auto result = StubbedTomlGatewayConfigParser::validate(*toml);
    ASSERT_EQ(charactersValidity.second, result.has_error());
    if (result.has_error())
    {
        EXPECT_EQ(TomlGatewayConfigParseError::INVALID_SERVICE_DESCRIPTION, result.get_error());
    }
}

TEST_P(TomlGatewayConfigParserSuiteTest, CheckCharactersUsedForServiceDescriptionToParseInTomlConfigFile)
{
    ::testing::Test::RecordProperty("TEST_ID", "9fc62870-448d-4ccb-b8a0-be76884841fb");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntry = cpptoml::make_table();
    const auto charactersValidity = GetParam();

    std::string stringentry = charactersValidity.first;
    serviceEntry->insert("service", stringentry);
    serviceEntry->insert("instance", stringentry);
    serviceEntry->insert("event", stringentry);
    serviceArray->push_back(serviceEntry);
    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_EQ(charactersValidity.second, result.has_error());
    if (!result.has_error())
    {
        GatewayConfig config = result.value();
        EXPECT_FALSE(config.m_configuredServices.empty());
    }
    else
    {
        EXPECT_EQ(TomlGatewayConfigParseError::INVALID_SERVICE_DESCRIPTION, result.get_error());
    }
}

TEST_F(TomlGatewayConfigParserSuiteTest, NoServiceNameInServiceDescriptionReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "fbdf3cdd-133c-4689-a73c-1ee2976b6726");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoServiceName = cpptoml::make_table();
    serviceEntryNoServiceName->insert("instance", "instance");
    serviceEntryNoServiceName->insert("event", "event");
    serviceArray->push_back(serviceEntryNoServiceName);
    toml->insert("services", serviceArray);

    auto result = StubbedTomlGatewayConfigParser::validate(*toml);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest, NoInstanceNameInServiceDescriptionReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "5b7382a1-7f78-4725-82b1-2508299719cc");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoInstanceName = cpptoml::make_table();
    serviceEntryNoInstanceName->insert("service", "service");
    serviceEntryNoInstanceName->insert("event", "event");
    serviceArray->push_back(serviceEntryNoInstanceName);
    toml->insert("services", serviceArray);

    auto result = StubbedTomlGatewayConfigParser::validate(*toml);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest, NoEventNameInServiceDescriptionReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "44c56d0a-daa4-4ea5-b889-7b3758ec5c59");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoEventName = cpptoml::make_table();
    serviceEntryNoEventName->insert("service", "service");
    serviceEntryNoEventName->insert("instance", "instance");
    serviceArray->push_back(serviceEntryNoEventName);
    toml->insert("services", serviceArray);

    auto result = StubbedTomlGatewayConfigParser::validate(*toml);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest, NoServicesInConfigReturnIncompleteConfigurationError)
{
    ::testing::Test::RecordProperty("TEST_ID", "14a75eaf-7eac-4ccd-a797-79dca9f382fc");
    auto toml = cpptoml::make_table();

    auto result = iox::config::StubbedTomlGatewayConfigParser::validate(*toml);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_CONFIGURATION, result.get_error());
}

// Without argument the iceoryx default config in /etc/iceoryx/gateway_config.toml is used. Then this
// test fails on every machine which is using such a config.
TEST_F(TomlGatewayConfigParserSuiteTest, DISABLED_ParseWithoutParameterTakeDefaultPathReturnNoError)
{
    ::testing::Test::RecordProperty("TEST_ID", "f18a7245-c4d5-4ad2-a74e-a622103f45f3");
    auto result = TomlGatewayConfigParser::parse();
    ASSERT_FALSE(result.has_error());

    GatewayConfig config = result.value();
    EXPECT_TRUE(config.m_configuredServices.empty());
}

TEST_F(TomlGatewayConfigParserSuiteTest, ParseWithEmptyPathReturnEmptyConfig)
{
    ::testing::Test::RecordProperty("TEST_ID", "1dc2aec6-31ab-4748-8e5f-f44be9777dcd");
    iox::roudi::ConfigFilePathString_t path = "";

    auto result = TomlGatewayConfigParser::parse(path);
    GatewayConfig config = result.value();

    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(config.m_configuredServices.empty());
}

TEST_F(TomlGatewayConfigParserSuiteTest,
       ParseWithoutServiceNameInServiceDescriptionInTomlConfigFileReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "c789eb60-935a-454d-95de-5083c0288a0a");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoServiceName = cpptoml::make_table();
    serviceEntryNoServiceName->insert("instance", "instance");
    serviceEntryNoServiceName->insert("event", "event");
    serviceArray->push_back(serviceEntryNoServiceName);
    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest,
       ParseWithoutInstanceNameInServiceDescriptionInTomlConfigFileReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "d40950c8-4be0-4b48-9188-e18d46e21225");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoInstanceName = cpptoml::make_table();
    serviceEntryNoInstanceName->insert("service", "service");
    serviceEntryNoInstanceName->insert("event", "event");
    serviceArray->push_back(serviceEntryNoInstanceName);
    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest,
       ParseWithoutEventNameInServiceDescriptionInTomlConfigFileReturnIncompleteServiceDescriptionError)
{
    ::testing::Test::RecordProperty("TEST_ID", "b81898dc-8f84-475d-af5b-5095abd31a15");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntryNoEventName = cpptoml::make_table();
    serviceEntryNoEventName->insert("service", "service");
    serviceEntryNoEventName->insert("instance", "instance");
    serviceArray->push_back(serviceEntryNoEventName);
    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_SERVICE_DESCRIPTION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest,
       ParseWithoutServicesConfigurationInTomlConfigFileReturnIncompleteConfigurationError)
{
    ::testing::Test::RecordProperty("TEST_ID", "e2a712d9-7f8b-45e2-a6a0-e16e8990c844");
    auto toml = cpptoml::make_table();
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(TomlGatewayConfigParseError::INCOMPLETE_CONFIGURATION, result.get_error());
}

TEST_F(TomlGatewayConfigParserSuiteTest, DuplicatedServicesDescriptionInTomlFileReturnOnlyOneEntry)
{
    ::testing::Test::RecordProperty("TEST_ID", "093f09d6-67ab-4da2-933f-e20fb5c42444");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    auto serviceEntry = cpptoml::make_table();
    serviceEntry->insert("service", "service");
    serviceEntry->insert("instance", "instance");
    serviceEntry->insert("event", "event");

    auto serviceEntry1 = cpptoml::make_table();
    serviceEntry1->insert("service", "service");
    serviceEntry1->insert("instance", "instance");
    serviceEntry1->insert("event", "event");
    serviceArray->push_back(serviceEntry1);

    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);
    GatewayConfig config = result.value();
    EXPECT_FALSE(result.has_error());
    EXPECT_FALSE(config.m_configuredServices.empty());
    EXPECT_EQ(config.m_configuredServices.size(), 1);
}

TEST_F(TomlGatewayConfigParserSuiteTest, ParseValidConfigFileWithMaximumAllowedNumberOfConfiguredServicesReturnNoError)
{
    ::testing::Test::RecordProperty("TEST_ID", "979101e3-764e-484f-aa6e-94b5c1cc0b5d");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();

    for (uint32_t i = 1U; i <= iox::MAX_GATEWAY_SERVICES; ++i)
    {
        auto serviceEntry = cpptoml::make_table();
        std::string stringentry = "validservice" + std::to_string(i);
        serviceEntry->insert("service", stringentry);
        serviceEntry->insert("instance", stringentry);
        serviceEntry->insert("event", stringentry);
        serviceArray->push_back(serviceEntry);
    }

    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);
    GatewayConfig config = result.value();

    EXPECT_EQ(config.m_configuredServices.size(), iox::MAX_GATEWAY_SERVICES);
    EXPECT_FALSE(result.has_error());
    EXPECT_FALSE(config.m_configuredServices.empty());

    uint32_t count = 1;
    for (auto configuredService : config.m_configuredServices)
    {
        std::string stringentry = "validservice" + std::to_string(count);
        EXPECT_EQ(configuredService.m_serviceDescription.getServiceIDString(), stringentry);
        EXPECT_EQ(configuredService.m_serviceDescription.getInstanceIDString(), stringentry);
        EXPECT_EQ(configuredService.m_serviceDescription.getEventIDString(), stringentry);
        ++count;
    }
}

TEST_F(TomlGatewayConfigParserSuiteTest,
       ParseValidConfigFileWithMoreThanMaximumAllowedNumberOfConfiguredServicesReturnError)
{
    ::testing::Test::RecordProperty("TEST_ID", "5fd22d76-1d13-4364-8fd7-2f5d434714f4");
    auto toml = cpptoml::make_table();
    auto serviceArray = cpptoml::make_table_array();
    auto serviceEntry = cpptoml::make_table();

    for (uint32_t i = 1U; i <= iox::MAX_GATEWAY_SERVICES + 1U; ++i)
    {
        std::string stringentry = "validservice" + std::to_string(i);
        serviceEntry->insert("service", stringentry);
        serviceEntry->insert("instance", stringentry);
        serviceEntry->insert("event", stringentry);
        serviceArray->push_back(serviceEntry);
    }


    toml->insert("services", serviceArray);
    CreateTmpTomlFile(toml);

    auto result = TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), MAXIMUM_NUMBER_OF_ENTRIES_EXCEEDED);
}

INSTANTIATE_TEST_SUITE_P(
    ParseAllMalformedInputConfigFiles,
    TomlGatewayConfigParserTest,
    Values(ParseErrorInputFile_t{iox::config::TomlGatewayConfigParseError::INVALID_SERVICE_DESCRIPTION,
                                 "generated_gateway_config.toml"},
           ParseErrorInputFile_t{iox::config::TomlGatewayConfigParseError::EXCEPTION_IN_PARSER,
                                 "toml_parser_exception.toml"}));


TEST_P(TomlGatewayConfigParserTest, ParseMalformedInputFileCausesError)
{
    ::testing::Test::RecordProperty("TEST_ID", "46f32eaf-b4d5-4ae1-b57e-aa23fcfcd2d5");
    const auto parseErrorInputFile = GetParam();

    m_configFilePath.append(iox::cxx::TruncateToCapacity, parseErrorInputFile.second);

    auto result = iox::config::TomlGatewayConfigParser::parse(m_configFilePath);

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(parseErrorInputFile.first, result.get_error());
}
