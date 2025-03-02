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

//! [iceoryx includes]
#include "request_and_response_types.hpp"

#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/client.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
//! [iceoryx includes]

#include <iostream>

constexpr char APP_NAME[] = "iox-cpp-request-response-client-waitset";


struct ContextData
{
    uint64_t fibonacciLast = 0;
    uint64_t fibonacciCurrent = 1;
    int64_t requestSequenceId = 0;
    int64_t expectedResponseSequenceId = requestSequenceId;
};

int main()
{
    //! [initialize runtime]
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    ContextData ctx;

    //! [create waitset]
    iox::popo::WaitSet<> waitset;

    //! [create client]
    iox::popo::ClientOptions options;
    options.responseQueueCapacity = 2U;
    iox::popo::Client<AddRequest, AddResponse> client({"Example", "Request-Response", "Add"}, options);
    //! [create client]

    // attach client to waitset
    waitset.attachState(client, iox::popo::ClientState::HAS_RESPONSE).or_else([](auto) {
        std::cerr << "failed to attach client" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    //! [create waitset]

    //! [mainloop]
    while (!iox::posix::hasTerminationRequested())
    {
        //! [send request]
        client.loan()
            .and_then([&](auto& request) {
                request.getRequestHeader().setSequenceId(ctx.requestSequenceId);
                ctx.expectedResponseSequenceId = ctx.requestSequenceId;
                ctx.requestSequenceId += 1;
                request->augend = ctx.fibonacciLast;
                request->addend = ctx.fibonacciCurrent;
                std::cout << APP_NAME << " Send Request: " << ctx.fibonacciLast << " + " << ctx.fibonacciCurrent
                          << std::endl;
                request.send().or_else(
                    [&](auto& error) { std::cout << "Could not send Request! Error: " << error << std::endl; });
            })
            .or_else([](auto& error) { std::cout << "Could not allocate Request! Error: " << error << std::endl; });
        //! [send request]


        // We block and wait for samples to arrive, when the time is up we send the request again
        auto notificationVector = waitset.timedWait(iox::units::Duration::fromSeconds(5));

        for (auto& notification : notificationVector)
        {
            if (notification->doesOriginateFrom(&client))
            {
                //! [take response]
                while (client.take().and_then([&](const auto& response) {
                    auto receivedSequenceId = response.getResponseHeader().getSequenceId();
                    if (receivedSequenceId == ctx.expectedResponseSequenceId)
                    {
                        ctx.fibonacciLast = ctx.fibonacciCurrent;
                        ctx.fibonacciCurrent = response->sum;
                        std::cout << APP_NAME << " Got Response : " << ctx.fibonacciCurrent << std::endl;
                    }
                    else
                    {
                        std::cout << "Got Response with outdated sequence ID! Expected = "
                                  << ctx.expectedResponseSequenceId << "; Actual = " << receivedSequenceId
                                  << "! -> skip" << std::endl;
                    }
                }))
                {
                }
                //! [take response]
            }
        }
        constexpr std::chrono::milliseconds SLEEP_TIME{950U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    //! [mainloop]

    std::cout << "shutting down" << std::endl;

    return (EXIT_SUCCESS);
}
