// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_binding_c/internal/c2cpp_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_enum_translation.hpp"
#include "iceoryx_binding_c/internal/cpp2c_service_description_translation.hpp"
#include "iceoryx_hoofs/cxx/requires.hpp"
#include "iceoryx_posh/popo/untyped_server.hpp"

using namespace iox;
using namespace iox::popo;
using namespace iox::runtime;
using namespace iox::capro;
using namespace iox::cxx;

extern "C" {
#include "iceoryx_binding_c/server.h"
}

#include <cstring>
constexpr uint64_t SERVER_OPTIONS_INIT_CHECK_CONSTANT = 333333331737373;

void iox_server_options_init(iox_server_options_t* const options)
{
    iox::cxx::Expects(options != nullptr);

    ServerOptions serverOptions;
    options->requestQueueCapacity = serverOptions.requestQueueCapacity;
    strncpy(options->nodeName, serverOptions.nodeName.c_str(), IOX_CONFIG_NODE_NAME_SIZE);
    options->offerOnCreate = serverOptions.offerOnCreate;
    options->requestQueueFullPolicy = cpp2c::queueFullPolicy(serverOptions.requestQueueFullPolicy);
    options->clientTooSlowPolicy = cpp2c::consumerTooSlowPolicy(serverOptions.clientTooSlowPolicy);
    options->initCheck = SERVER_OPTIONS_INIT_CHECK_CONSTANT;
}

bool iox_server_options_is_initialized(const iox_server_options_t* const options)
{
    iox::cxx::Expects(options != nullptr);

    return options->initCheck == SERVER_OPTIONS_INIT_CHECK_CONSTANT;
}

iox_server_t iox_server_init(iox_server_storage_t* self,
                             const char* const service,
                             const char* const instance,
                             const char* const event,
                             const iox_server_options_t* const options)
{
    iox::cxx::Expects(self != nullptr);
    iox::cxx::Expects(service != nullptr);
    iox::cxx::Expects(instance != nullptr);
    iox::cxx::Expects(event != nullptr);
    iox::cxx::Expects(options == nullptr || (options != nullptr && iox_server_options_is_initialized(options)));

    ServerOptions serverOptions;
    if (options != nullptr)
    {
        serverOptions.requestQueueCapacity = options->requestQueueCapacity;
        serverOptions.nodeName = iox::NodeName_t(TruncateToCapacity, options->nodeName);
        serverOptions.offerOnCreate = options->offerOnCreate;
        serverOptions.requestQueueFullPolicy = c2cpp::queueFullPolicy(options->requestQueueFullPolicy);
        serverOptions.clientTooSlowPolicy = c2cpp::consumerTooSlowPolicy(options->clientTooSlowPolicy);
    }

    new (self) UntypedServer(ServiceDescription{IdString_t(TruncateToCapacity, service),
                                                IdString_t(TruncateToCapacity, instance),
                                                IdString_t(TruncateToCapacity, event)},
                             serverOptions);

    return reinterpret_cast<iox_server_t>(self);
}

void iox_server_deinit(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    self->~UntypedServer();
}

iox_ServerRequestResult iox_server_take_request(iox_server_t const self, const void** const payload)
{
    iox::cxx::Expects(self != nullptr);
    iox::cxx::Expects(payload != nullptr);

    auto result = self->take();
    if (result.has_error())
    {
        return cpp2c::serverRequestResult(result.get_error());
    }
    *payload = result.value();
    return ServerRequestResult_SUCCESS;
}

void iox_server_release_request(iox_server_t const self, const void* const payload)
{
    iox::cxx::Expects(self != nullptr);
    iox::cxx::Expects(payload != nullptr);

    self->releaseRequest(payload);
}

iox_AllocationResult iox_server_loan_response(iox_server_t const self,
                                              const void* const requestPayload,
                                              void** const payload,
                                              const uint32_t payloadSize)
{
    return iox_server_loan_aligned_response(
        self, requestPayload, payload, payloadSize, IOX_C_CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT);
}

iox_AllocationResult iox_server_loan_aligned_response(iox_server_t const self,
                                                      const void* const requestPayload,
                                                      void** const payload,
                                                      const uint32_t payloadSize,
                                                      const uint32_t payloadAlignment)
{
    iox::cxx::Expects(self != nullptr);
    iox::cxx::Expects(requestPayload != nullptr);
    iox::cxx::Expects(payload != nullptr);

    auto result = self->loan(RequestHeader::fromPayload(requestPayload), payloadSize, payloadAlignment);
    if (result.has_error())
    {
        return cpp2c::allocationResult(result.get_error());
    }

    *payload = result.value();
    return AllocationResult_SUCCESS;
}

iox_ServerSendResult iox_server_send(iox_server_t const self, void* const payload)
{
    iox::cxx::Expects(self != nullptr);

    auto result = self->send(payload);
    if (result.has_error())
    {
        return cpp2c::serverSendResult(result.get_error());
    }

    return ServerSendResult_SUCCESS;
}

void iox_server_release_response(iox_server_t const self, void* const payload)
{
    iox::cxx::Expects(self != nullptr);
    iox::cxx::Expects(payload != nullptr);

    self->releaseResponse(payload);
}

iox_service_description_t iox_server_get_service_description(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    return TranslateServiceDescription(self->getServiceDescription());
}

void iox_server_offer(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    self->offer();
}

void iox_server_stop_offer(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    self->stopOffer();
}

bool iox_server_is_offered(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    return self->isOffered();
}

bool iox_server_has_clients(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    return self->hasClients();
}

bool iox_server_has_requests(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    return self->hasRequests();
}

bool iox_server_has_missed_requests(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    return self->hasMissedRequests();
}

void iox_server_release_queued_requests(iox_server_t const self)
{
    iox::cxx::Expects(self != nullptr);

    self->releaseQueuedRequests();
}
