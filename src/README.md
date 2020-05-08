# Events and Services Framework for Arduino  
 
The Events and Services Framework allows projects to be created with up to 16 individual services that can operate as state machines or simple run functions. The framework continuously checks for events coded by the user and sends events to the designated service(s) when they are registered. Events can be the change in state of a gpio pin, variable, timer, etc.

The framework currently does not operate on an order of priority of services. Events are handled entire on a FIFO basis. 

This is a modified Arduino implementation of the Stanford ME 218 Events and Services Framework. 

To use the framework, use the template service file to create a new service with its own init(), post(), and run() function. Then, register this service in ES_Configure.h by incrementing NUM_SERVICES and adding the header file along with init and run function into the next available services section. Lastly, add the event checker functions associated with this services to EVENT_CHECKER_LIST. 

When an event is registered by an event checker function, it should submit that event to the appropriate service by calling that service's post function with the custom ES_event_t. ES_event_t events can be created by adding your event name into the typedef enum in ES_configure.h. 
A functioning dummy service, KeyboardService, is provided as an example. It just echoes back the char typed into the serial monitor.   

 