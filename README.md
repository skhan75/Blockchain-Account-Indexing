# Blockchain Account Indexing
This project demonstrates the indexing and management of account updates in a blockchain system. It processes JSON files containing account updates, indexes the accounts based on specified criteria, and manages callbacks for the accounts.

## Prerequisites
To compile and run the project, you need the following:

* C++ compiler that supports C++11 standard
* The nlohmann/json.hpp library (already included in the project)

## Building and Running the Project
To run the code in a local environment, follow these steps:

1. Clone the repository or download the source code files.
2. Make sure you have a C++ compiler installed on your system.
3. Compile the code by running the following command:
`$make`.
    * You'll see some warnings, but you can safely ignore them
4. Run the compiled executable:
`./blockchain_account_manager`

## Design Patterns
The project utilizes the following design patterns:

* **Observer Pattern**: The CallbackManager class acts as the subject, managing callbacks and notifying registered observers (callbacks) when a callback's scheduled time is reached.

* **Priority Queue**: The AccountIndexer class uses a priority queue to store and retrieve accounts based on their token values. This allows efficient retrieval of the highest token value accounts.

## Observability in Production
If this project were to be deployed in an actual production environment, the following observability measures can be added, which I have skipped for the implementation sample of the project:

* **Logging**: Implement comprehensive logging throughout the codebase to record important events, errors, and state changes. This will help in debugging and monitoring the system.

* **Metrics**: Collect relevant metrics to monitor the system's performance and behavior. For example, track the number of indexed accounts, the frequency of callback fires, and the processing time for account updates.

* **Error Handling and Reporting**: Enhance the error handling mechanisms to capture and report any unexpected errors or exceptions that occur during runtime. This information can be logged or sent to a centralized monitoring system.

* **Tracing**: Implement distributed tracing to track the flow of requests and identify bottlenecks or performance issues. This can help identify the source of delays or failures in processing account updates and callbacks.

* **Alerting**: Set up alerting mechanisms to notify administrators or operators when specific events or conditions are met. For example, send an alert if the number of pending callbacks exceeds a certain threshold or if an error rate exceeds a defined limit.

* **Enums**: A good set of Enums also enhances the code by disabling manual adding/modifying names and strings, and moving them to a an Enum file.

By incorporating these observability measures, the production system can be effectively monitored, debugged, and optimized to ensure reliable and efficient account indexing and callback management.