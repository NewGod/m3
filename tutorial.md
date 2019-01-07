# M3-Lab3 Tutorial

## Description & Installation

See `README.md`

## Structure

M3 builds TCP connections between proxy and relay, and forwards packets between client and server.  
M3 has 6/7 individual threads containing infinite loop, each has its own job to make M3 works properly. I will give a brief introduction and the structure will be clear to you.

- ConnectionClassifier: Repeatly capture packets, classify which connection it belongs to, and insert the packets into the buffer of the connection.
- ConnectionManager: Maintain connection set, select connection to serve, and remove expired/closed connection periodically.
- InterfaceManager: Consume packets from connections, schedule interface, and send the packets on the interface(s).
- NaiveTCPStreamWatcher: Capture the packets between proxy and relay, and use them to renew metrics
- ForwardModule: Handle kernel message (message generated and consumed by M3).
- AsynchronousTCPContext: Repeatly read TCP socket information from kernel. Only available when macro `ENABLE_CWND_AWARE` defined.
- DatabaseProvider: Read and provide the historical data of TCP metrics. Only available when macro `BARE_FRAMEWORK` **NOT** defined.

## Goal
You need to implement your own inter-connection scheduler (see ConnectionManager) and interface scheduler (see InterfaceManager), and related metric provider.

## Modules you may use
- **Scheduler**(.hh): Declares the prototype of the interface scheduler. All the interface scheduler should be inherited from this class. Generally, the `schedule` method is the only thing you need to modify.
    - `schedule`:  
        - `Interface**`: store the interfaces you want to use into this array
        - `Message*`: the message you are going to send
        - `int`: return the number of interfaces you select
        - Read `RRScheduler.hh` for details

- Interface(.hh/.cc): The interface structure, contains file descriptor of the socket, MSS, MTU, etc.. **You should NOT modify any part of this class**
    - `getProvider`: get metric provider of this interface.
    - `aTCPCxt`: static pointer to `AsynchronousTCPContext`, only valid when macro `ENABLE_CWND_AWARE` is defined. It will repeatly read the informations of this TCP connection from kernel. You can call `getInfo` method to get the tcp_info (file descriptor is stored in Interface as `fd`).

- MetricProvider(.hh): Maintain the metrics (inherited from `ProtocolMetric`). M3 provide `ArrayMetricProvider` and `TCPBasicMetric`, it is enough generally, but you can add other metrics.
    - `getMetric`: get metric by its ID. For `TCPBasicMetric`, the ID is `ProtocolMetric::Type::TCP_BASIC`.

- **TCPBasicMetric**(.hh/.cc): Provide methods to maintain the TCP metrics.
    - `getValue`: get the metric value structure (`TCPBasicMetricValue`). Note that the value is read-only, and can only be modified by `renew` method in `TCPBasicMetric`.
    - `acquireValue`: same as `getValue`, but the metric will be  protected by a read-write lock, so that you can use the metric without considering the race condition caused by `renew`. Note that you should call `releaseValue` after reading.
    - Read the comments for information of each metric value
    - If you want to add other metrics:  
        Declare the metric in `TCPBasicMetricValue`  
        Modify `renew` method to maintain the metric

- **DatabaseProvider**(.hh/.cc): Provide the historical data of TCP metrics.
    - DatabaseProvider should be accessed by the global pointer `DatabaseProvider *dbprovider`, you need to declare it as `extern`, or just include `DatabaseProvider.hh`.
    - It provides the historical data (throughput, RTT and loss rate) of each interface, the position (of the last time received message) and predicted velocity of relay.
    - `acquireMeta`: get the metadata, you should call `releaseMeta` after reading.
        - Database stores the nearest three data (of the point just passed, the point not reached, the next point to reach) read from database (include GPS value, and metrics). You can use interplotation to get more accuracy data.
        - `DBRef` returns the reference of the metrics at point `idx` (0,1,2)
        - The metrics of interface `i` is stored in `DBRef(j).value[i]`

- **ConnectionSetHandler**(.hh): Maintain connections of the same priority and schedule them to serve.
    - Read `ConnectionSetHandler.hh` for details and `RRConnectionSetHandler.hh/.cc` for sample.

- *ConnectionMetadata*(.hh/.cc): Maintain information for each connection.
    - `startTime`: time stamp of the starting time of this connection
    - `session`: holds all the connections with the same destination. It has also a meta data, and you can store some information you need.

- *ConnectionType*(.hh): The type of connection, for example, video, web browsing, etc. The type is infered from `HTTPDetector`, you need to add your own inference rule in it if needed.
    - To add one ConnectionType, construct a child class inherited from `ConnectionType`
    - Implement getType, getDetector method. The getDetector method should return the HTTPDetector that construct this type
    - Add inference logic in `HTTPDetector::setTypeForResponse`

## How to implement a scheduler
- Inter-Connection scheduler
    - Construct a child class inherit from `ConnectionSetHandler`
    - Maintain a set of connection pointers (it does **NOT** mean that you must use `std::set`)
    - Implement `addConnection` method used for insert one Connection into the set, return `0` if success
    - Implement `size` method that returns the number of connections in the set
    - Implement at least one of `gc` and `removeConnection` methods used for releasing resource from closed connections
        - `removeConnection` remove the target connection from set, return `0` if success, else return `1`
        - If `removeConnection` failed, ConnectionManager will call `gc` to release all the closed connections (every 10 seconds)
        - `gc` should remove all the connections, state of which is `TCPStateMachine::CLOSED`. `gc` should never fail
        - Call `Connection::release` after removing the connection from set
    - Implement `next` method, choose one connection from the set that will be scheduled
        - The connection **MUST** have at least one packet waiting in buffer, use `Connection::hasNext` method to check if there are packets in buffer.
        - Otherwise all the connections at the same priority will be ignored.
        - Return `0` if success, else `1`
    - All the methods should be **thread-safe**
    - The ConnectionMetadata will be renewed in following situation, you can use the ConnectionMetadata to do connection scheduling and reordering.
        - ConnectionSetHandler select one connection to schedule (`renewOnReorderSchedule`)
        - Data in (`renewOnInput`)
        - Packet sent (`renewOnOutput`)
    - Read `RRConnectionSetHandler.hh` and `ConnectionMetadata.hh/.cc` for details

- Interface scheduler
    - Construct a child class inherit from `Scheduler`
    - Implement `schedule` method that selects interface(s) for the message to send
        - This method takes an array of Interface pointers and one Message pointer.
        - Use `Message::getConnection` to get connection and then use `Connection::getMeta` to load ConnectionMetadata, or `Connection::getType` to load ConnectionType.
        - Use `Interface::getProvider` and `MetricProvider::getMetric` to load TCPBasicMetric.
        - Use `Interface::aTCPCxt` to load `tcp_info` read from kernel, you can change the frequency of trapping in the constructor of `Interface`
        - Use these metrics (and/or other information) to select interface(s) to use
        - Check if `Interface::isReady` before selecting one interface
        - Store the interface(s) you choose into the array, and returns the number of interface(s) you choose.

- Enable your scheduler
    - Modify `PolicyManager::assignDefaultPolicy` to enable your interface scheduler. (For connection of `REALTIME` priority, `RRScheduler` is still suggested)
    - Modify `Proxy.cc` and `MobileRelay.cc` to enable your inter-connection scheduler.

## How to debug
- Never ignore error code
- Use `logMessage`, `logWarning`, `logError` to print necessary messages.
- Define `ENABLE_DEBUG_LOG` macro and use `logDebug` to print debug messages.  
**NOTE**: This will cause performance decreasing for too many outputs.

## Reflection

M3 has implemented a simple runtime reflection machanism (but only used in compile-time currently). You can use reflection to make your code more modulized and more flexible.  
To add reflection support of your code, you need to:

- Declare `newInstance` method to your class and implement it when declaring. It should be `static`, take no parameters, and return a pointer to new instance of the class with type `void*`.
- Append comment `//@ReflectXXX` to the line declaring `newInstance`, where `XXX` is the reflection name.

For example:
```
static void* newInstance() //@ReflectClassA
{
    return (void*) new ClassA();  
}
```
