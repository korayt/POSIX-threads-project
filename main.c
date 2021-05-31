#include <stdint.h> // uint32
#include <stdlib.h> // exit, perror
#include <unistd.h> // usleep
#include <stdio.h> // printf
#include <pthread.h> // pthread_*
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>


#ifdef __APPLE__
    #include <dispatch/dispatch.h>
    typedef dispatch_semaphore_t psem_t;
#else
    #include <semaphore.h> // sem_*
    typedef sem_t psem_t;
#endif

void psem_init(psem_t *sem, uint32_t value);
void psem_wait(psem_t sem);
void psem_post(psem_t sem);


void lock(psem_t sem);
void unlock(psem_t sem);

psem_t n;
psem_t m;
psem_t t;

int id = 0;
int commNum = 0;
int tspeak = 3;
int numberOfQuestions = 5;
int queuePosition = 0;
int isBreaking = 0;
int remainingNumberOfQuestions;
int numberOfCommentators = 4;
int questionAsked = 0;
int commentatorSpeaking = 0;

double probOfBreakingNews = 0.05;
double probOfAnswering = 0.75;

struct timeval start, end;
struct Queue* queue;

pthread_t tid[1024] = {0}; // Max number of threads
int thread_count = 0;

long* getTime();


void create_new_thread(void *(func)(void* vargp))
{
    pthread_create(&tid[thread_count++], NULL, func, NULL);
}

//QUEUE IMPLEMENTATION
// A linked list (LL) node to store a queue entry
struct QNode {
    psem_t key;
    struct QNode* next;
};
  
// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue {
    struct QNode *front, *rear;
};
  
// A utility function to create a new linked list node.
struct QNode* newNode(psem_t k)
{
    struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode));
    temp->key = k;
    temp->next = NULL;
    return temp;
}
  
// A utility function to create an empty queue
struct Queue* createQueue()
{
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}
  
// The function to add a key k to q
void enQueue(struct Queue* q, psem_t k)
{
    // Create a new LL node
    struct QNode* temp = newNode(k);
  
    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
  
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}
  
// Function to remove a key from given queue q
psem_t deQueue(struct Queue* q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return NULL;
  
    // Store previous front and move front one node ahead
    struct QNode* temp = q->front;
  
    q->front = q->front->next;
  
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
    
    psem_t dummy = temp->key;
    free(temp);
    return dummy;
}

bool isEmpty(struct Queue* q){
    return q->rear == NULL;
}

//QUEUE IMPLEMENTATION

void init(psem_t *sem)
{
    psem_init(sem, 1);

}
// Acquire a lock
void lock(psem_t sem)
{
    psem_wait(sem);

}

// Release an acquired lock
void unlock(psem_t sem)
{
    psem_post(sem);

}

int pthread_sleep(double seconds){
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    if(pthread_mutex_init(&mutex,NULL)){
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL)){
        return -1;
    }

    struct timeval tp;
    struct timespec timetoexpire;
    // When to expire is an absolute time, so get the current time and add
    // it to our delay time
    gettimeofday(&tp, NULL);
    long new_nsec = tp.tv_usec * 1000 + (seconds - (long)seconds) * 1e9;
    timetoexpire.tv_sec = tp.tv_sec + (long)seconds + (new_nsec / (long)1e9);
    timetoexpire.tv_nsec = new_nsec % (long)1e9;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}

void askQuestion(){
    long* time = getTime();
    printf("[%02ld:%02ld:%03ld] Moderator asked Question %d\n", time[0], time[1], time[2], (numberOfQuestions - remainingNumberOfQuestions)+1);
    questionAsked = 1;
    remainingNumberOfQuestions--;
}

double randfrom(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void answerQuestion(int id){
    commentatorSpeaking = id;
    double speakTime = randfrom(1, tspeak);
    long* time = getTime();
    printf("[%02ld:%02ld:%03ld] Commentator #%d’s turn to speak for %f seconds\n", time[0], time[1], time[2], id, speakTime);
    pthread_sleep(speakTime);
    if (isBreaking){
        lock(t);
        return;
    }
    time = getTime();
    printf("[%02ld:%02ld:%03ld] Commentator #%d finished speaking\n", time[0], time[1], time[2], id);
}

int hasAnswer(){
    int r = (rand() % 100)+1;
//    printf("%d\n",r);
    if(r<=probOfAnswering*100){
        return 1;
    }
    return 0;
}

int isGoingToBreak(){
    int r = (rand() % 100)+1;
    if(r<=probOfBreakingNews*100){
        return 1;
    }
    return 0;
}

void *moderator(){
    lock(n);
    while(remainingNumberOfQuestions || !isEmpty(queue)){
        if (isEmpty(queue)) {
            askQuestion();
            lock(n);
        }else{
            unlock(deQueue(queue));
            lock(n);
        }
    }
    return 0;
}

void *breakingNews(){
    
    while(remainingNumberOfQuestions || questionAsked){
        if (isBreaking) {
            long* time = getTime();
            printf("[%02ld:%02ld:%03ld] Breaking News!\n", time[0], time[1], time[2]);
            time = getTime();
            printf("[%02ld:%02ld:%03ld] Commentator #%d cut short due to a breaking news\n", time[0], time[1], time[2], commentatorSpeaking);
            pthread_sleep(5);
            time = getTime();
            printf("[%02ld:%02ld:%03ld] Breaking News ended!\n", time[0], time[1], time[2]);
            isBreaking = 0;
            unlock(t);
        }
    }
    return 0;
}
  

void *commentator(){
    psem_t turn;
    int wantsToAnswer = 0;
    lock(m);
    int commentatorNumber = id++;
    unlock(m);
    init(&turn);
    lock(turn);
    while(remainingNumberOfQuestions || questionAsked){
        if(questionAsked){
            lock(m);
            if (hasAnswer() && questionAsked){
                wantsToAnswer = 1;
                enQueue(queue, turn);
                long* time = getTime();
                printf("[%02ld:%02ld:%03ld] Commentator #%d generates answer, position in queue: %d\n", time[0], time[1], time[2],commentatorNumber, queuePosition);
                queuePosition++;
                commNum++;
            }else if(questionAsked){
                commNum++;
            }
            if(commNum == numberOfCommentators){
                questionAsked = 0;
                queuePosition = 0;
                commNum = 0;
                unlock(n);
            }
            unlock(m);
            if (wantsToAnswer){
                lock(turn);
                answerQuestion(commentatorNumber);
                wantsToAnswer = 0;
                unlock(n);
            }
        }
    }
    return 0;
}

long* getTime(){
    gettimeofday(&end, NULL);
    long timeSec = (end.tv_sec - start.tv_sec);
    long timeMiliSec = ((end.tv_usec - start.tv_usec)/1000);
    long total = timeSec*1000 + timeMiliSec;
    long milisec = total % 1000;
    long totalSec = total / 1000;
    long min = totalSec / 60;
    long sec = totalSec % 60;
    long time[] = {min,sec,milisec};
    return time;
}


int main(int argc, char *argv[]){
    for( int i = 1; i < argc; i++ ){
        if(strcmp(argv[i], "-n" ) == 0 ){
           numberOfCommentators = atoi(argv[i+1]);
        }else if(strcmp(argv[i], "-p" ) == 0){
            probOfAnswering =atof(argv[i+1]);
        }else if(strcmp(argv[i], "-q" ) == 0){
            numberOfQuestions = atoi(argv[i+1]);
        }else if(strcmp(argv[i], "-t" ) == 0){
            tspeak = atoi(argv[i+1]);
        }else if(strcmp(argv[i], "-b" ) == 0){
            probOfBreakingNews = atof(argv[i+1]);
        }
    }
    remainingNumberOfQuestions = numberOfQuestions;
    queue = createQueue();
    questionAsked = 0;
    init(&n);
    init(&m);
    init(&t);
    lock(t);
    gettimeofday(&start, NULL);
    create_new_thread(breakingNews);
    for (int i = 0; i<numberOfCommentators;i++){
        create_new_thread(commentator);
    }
    create_new_thread(moderator);
    
    pthread_sleep(1);
    
    while(remainingNumberOfQuestions || questionAsked ){
        int r = (rand() % 100)+1;
        if(r<=probOfBreakingNews*100 && !isBreaking){
            isBreaking = 1;
        }else
            pthread_sleep(1);
    }
    
    for (int i=0; i<thread_count; ++i)
        pthread_join(tid[i], NULL);
    
    return 0;
}


#ifdef __APPLE__
void psem_init(psem_t *sem, uint32_t value) {
    *sem = dispatch_semaphore_create(value);
}
void psem_wait(psem_t sem) {
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}
void psem_post(psem_t sem) {
    dispatch_semaphore_signal(sem);
}
#else
void psem_init(psem_t *sem, u_int32_t value) {
    sem_init(sem, 0, value);
}
void psem_wait(psem_t sem) {
    sem_wait(&sem);
}
void psem_post(psem_t sem) {
    sem_post(&sem);
}
#endif

