all : Dispatcher_Worker_model

Dispatcher_Worker_model: Dispatcher_Worker_model.c
	gcc -g -Wall Dispatcher_Worker_model.c -o Dispatcher_Worker_model

clean:
	\rm Dispatcher_Worker_model
	\rm count*
	\rm thread*
	\rm stats*
	\rm dispatcher*
	
