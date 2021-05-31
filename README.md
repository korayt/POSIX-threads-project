# How Make file works:

open terminal at the folder in which main.c resides.
run "make" command with desired arguments.

Arguments: use N for number of commentators, P for probability of answering, Q for number of questions, T for speaking time, B for breaking news probability.

Sample run:
make N=3 P=0.8 Q=5 T=3 B=0.01

Default parameters are:
N=4, P=0.75 Q=5 T=3 B=0.05.

--- --- --- --- ---

# Our implementation

Commentator and moderators are in seperate threads and created at the start. 
Moderator asks a question and waits. 
QuestionAsked variable is know 1 and commentators go inside the if block and start to answer one by one which is ensured by another lock.
hasAnswer() method determines if a commentator wants to speak.
when commentator leaves the critical section if he wants to answer his lock is enqueued.
If he doesn't want to answer he waits in the while loop.
If a commentator's lock is enqueued it waits for the moderator's signal.
When every commentator is made their decision if they want to answer or not QuestionAsked variable turns to 0 and moderator is notified. 
Moderator dequeues until the queue is empty.
When a commentator's lock is dequeued moderator waits and that commentator is notified then commentator answers the question.
After commentator answered moderator is notified and dequeues if the queue is not empty.
When the list is empty moderator asks the next question.
After each question is answered program ends.

Breaking news is implemented with main thred and breaking news thred. 
Main thread determines if there is a breaking news event each second using the probability of breaking news parameter.
If there is a breaking news event it sleeps 
Breaking news thread checks if a breaking news event is happening, if it is lock the current speaker.
Breaking news thread then sleeps for 5 seconds.
After 5 seconds it unlocks the current speaker. 
Next speaker talks afterwards.

Every part works
--
