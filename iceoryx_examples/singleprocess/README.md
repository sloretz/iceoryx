# Single process

## Introduction

This example demonstrates how iceoryx can be used in a single process for
inter-thread communication. This is for instance helpful if you would like
to establish a simple communication API in a 3D engine with a multitude of
threads that are interacting without starting RouDi every time separately.

## Run singleprocess

[![asciicast](https://asciinema.org/a/407439.svg)](https://asciinema.org/a/407439)

The first lines until `RouDi is ready for clients` are coming from the RouDi
startup in which the shared memory management segment and user data segment are
created.

Afterward, the publisher and subscriber thread are started and are beginning to
transmit and receive data.

## Code Walkthrough

### Creating a Single Process RouDi, Publisher and Subscriber

 1. We start by setting the log level to error since we do not want to see all the
    debug messages.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][log level]-->
```cpp
iox::log::LogManager::GetLogManager().SetDefaultLogLevel(iox::log::LogLevel::kError);
```

 2. To start RouDi we have to create a configuration for him. We are choosing the
    default config. Additionally, RouDi needs some other components like a memory
    management unit which handles how the memory is created in which the transmission
    data is stored. The `IceOryxRouDiComponents` class is handling them for us

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][roudi config]-->
```cpp
iox::RouDiConfig_t defaultRouDiConfig = iox::RouDiConfig_t().setDefaults();
iox::roudi::IceOryxRouDiComponents roudiComponents(defaultRouDiConfig);
```

 3. We are starting RouDi and provide him with the required components. Furthermore, we
    disable monitoring. The last bool parameter `false` states that RouDi does not
    terminate all registered processes when he goes out of scope. If we would set it
    to `true`, our application would self terminate when the destructor is called.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][roudi]-->
```cpp
iox::roudi::RouDi roudi(roudiComponents.rouDiMemoryManager,
                        roudiComponents.portManager,
                        iox::roudi::RouDi::RoudiStartupParameters{iox::roudi::MonitoringMode::OFF, false});
```

 4. Here comes a key difference to an inter-process application. If you would like
    to communicate within one process, you have to use `PoshRuntimeSingleProcess`.
    You can create only one runtime at a time!

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][runtime]-->
```cpp
iox::runtime::PoshRuntimeSingleProcess runtime("singleProcessDemo");
```

 5. Now that everything is up and running, we can start the publisher and subscriber
    thread, wait two seconds and stop the example.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][run]-->
```cpp
std::thread publisherThread(publisher), subscriberThread(subscriber);

iox::posix::waitForTerminationRequest();

publisherThread.join();
subscriberThread.join();

std::cout << "Finished" << std::endl;
```

### Implementation of Publisher and Subscriber

Since there are no differences to inter-process ports you can take a look at the
[icedelivery example](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/icedelivery)
for detailed documentation. We only provide you here with a short overview.

#### Publisher

We create a typed publisher with the following service description
(Service = `Single`, Instance = `Process`, Event = `Demo`)

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][publisher]-->
```cpp
iox::popo::PublisherOptions publisherOptions;
publisherOptions.historyCapacity = 10U;
iox::popo::Publisher<TransmissionData_t> publisher({"Single", "Process", "Demo"}, publisherOptions);
```

After that, we are sending numbers in ascending order with a 100ms interval in a `while` loop.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][send]-->
```cpp
uint64_t counter{0};
constexpr const char GREEN_RIGHT_ARROW[] = "\033[32m->\033[m ";
while (!iox::posix::hasTerminationRequested())
{
    publisher.loan().and_then([&](auto& sample) {
        sample->counter = counter++;
        consoleOutput("Sending   ", GREEN_RIGHT_ARROW, sample->counter);
        sample.publish();
    });

    std::this_thread::sleep_for(CYCLE_TIME);
}
```

#### Subscriber

Like with the publisher, we are creating a corresponding subscriber port with the
same service description.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][subscriber]-->
```cpp
iox::popo::SubscriberOptions options;
options.queueCapacity = 10U;
options.historyRequest = 5U;
iox::popo::Subscriber<TransmissionData_t> subscriber({"Single", "Process", "Demo"}, options);
```

Now we can receive the data in a while loop till `keepRunning` is false. But we
only try to acquire data if our `SubscribeState` is `SUBSCRIBED`.

<!--[geoffrey][iceoryx_examples/singleprocess/single_process.cpp][receive]-->
```cpp
constexpr const char ORANGE_LEFT_ARROW[] = "\033[33m<-\033[m ";
while (!iox::posix::hasTerminationRequested())
{
    if (iox::SubscribeState::SUBSCRIBED == subscriber.getSubscriptionState())
    {
        bool hasMoreSamples{true};

        do
        {
            subscriber.take()
                .and_then([&](auto& sample) { consoleOutput("Receiving ", ORANGE_LEFT_ARROW, sample->counter); })
                .or_else([&](auto& result) {
                    hasMoreSamples = false;
                    if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                    {
                        std::cout << "Error receiving chunk." << std::endl;
                    }
                });
        } while (hasMoreSamples);
    }

    std::this_thread::sleep_for(CYCLE_TIME);
}
```

<center>
[Check out singleprocess on GitHub :fontawesome-brands-github:](https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_examples/singleprocess){ .md-button }
</center>
