#include <errno.h>
#include <wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
//cow
#define SEM_COWSINGROUP 0
#define SEM_PCOWSINGROUP 1
#define SEM_COWSWAITING 2
#define SEM_PCOWSEATEN 3
#define SEM_COWSEATEN 4
#define SEM_COWSDEAD 5
//sheep
#define SEM_SHEEPINGROUP 6
#define SEM_PSHEEPINGROUP 7
#define SEM_SHEEPWAITING 8
#define SEM_PSHEEPEATEN 9
#define SEM_SHEEPEATEN 10
#define SEM_SHEEPDEAD 11
//thief
#define SEM_THIEFWAITING 12
#define SEM_THIEFDEFEATED 13
#define SEM_PTHIEFWAITING 14
#define SEM_THIEFPLAYING 15
#define SEM_PTHIEFDEFEATED 16
#define SEM_PLAYOVER 30
#define SEM_PFIGHTRESULTS 31
#define SEM_PTHIEFWAITINGFLAG 34
//hunter
#define SEM_HUNTERSWAITING 17
#define SEM_HUNTERSDEFEATED 18
#define SEM_PHUNTERSWAITING 19
#define SEM_HUNTERFIGHTING 20
#define SEM_PHUNTERSDEFEATED 21
#define SEM_FIGHTOVER 32
#define SEM_PHUNTERWAITINGFLAG 33
//smaug
#define SEM_PTERMINATE 22
#define SEM_DRAGONEATING 23
#define SEM_DRAGONFIGHTING 24
#define SEM_DRAGONSLEEPING 25
#define SEM_DRAGONPLAYING 26
#define SEM_PMEALWAITINGFLAG 27

/* System constants used to control simulation termination */
//cow
#define MAX_COWS_EATEN 14
#define MAX_COWS_CREATED 80
//sheep
#define MAX_SHEEP_EATEN 14
#define MAX_SHEEP_CREATED 80
//treasure
#define MAX_TREASURE_IN_HOARD 1000
#define INITIAL_TREASURE_IN_HOARD 400
//thief
#define MAX_THIEVES_DEFEATED 15
//hunter
#define MAX_HUNTER_DEFEATED 12

/* System constants to specify size of groups of cows*/
#define COWS_IN_GROUP 2
#define SHEEP_IN_GROUP 2

/* CREATING YOUR SEMAPHORES */
int semID;

union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
} seminfo;

struct timeval startTime;

/*  Pointers and ids for shared memory segments */
int mealWaitingFlag = 0;
int fightOutcome = 0;
int terminateFlag = 0;
int *mealWaitingFlagp = NULL;
int *fightOutcomep = NULL;
int *terminateFlagp = NULL;
//cow
int cowCounter = 0;
int cowsEatenCounter = 0;
int *cowCounterp = NULL;
int *cowsEatenCounterp = NULL;
//sheep
int sheepCounter = 0;
int sheepEatenCounter = 0;
int *sheepCounterp = NULL;
int *sheepEatenCounterp = NULL;
//thief
int thiefWaiting = 0;
int defeatedThiefCount = 0;
int *thiefWaitingp = NULL;
int *defeatedThiefCountp = NULL;
//hunter
int hunterWaiting = 0;
int defeatedHunterCount = 0;
int *hunterWaitingp = NULL;
int *defeatedHunterCountp = NULL;

/* Group IDs for managing/removing processes */
//cow
int cowProcessGID = -1;
//sheep
int sheepProcessGID = -1;
//theif
int thiefProcessGID = -1;
//hunter
int hunterProcessGID = -1;
//smaug
int parentProcessGID = -1;
int smaugProcessID = -1;

//win percentage for visitors
int winProb = 0;
int *winProbp = NULL;

/* Define the semaphore operations for each semaphore*/
/* Arguments of each definition are: */
/* Name of semaphore on which the operation is done */
/* Increment (amount added to the semaphore when operation executes*/
/* Flag values (block when semaphore <0, enable undo ...)*/

/*Number in group semaphores*/
//cow
struct sembuf WaitCowsInGroup={SEM_COWSINGROUP, -1, 0};
struct sembuf SignalCowsInGroup={SEM_COWSINGROUP, 1, 0};
//sheep
struct sembuf WaitSheepInGroup={SEM_SHEEPINGROUP, -1, 0};
struct sembuf SignalSheepInGroup={SEM_SHEEPINGROUP, 1, 0};

/*Number in group mutexes*/
//cow
struct sembuf WaitProtectCowsInGroup={SEM_PCOWSINGROUP, -1, 0};
struct sembuf SignalProtectCowsInGroup={SEM_PCOWSINGROUP, 1, 0};
//sheep
struct sembuf WaitProtectSheepInGroup={SEM_PSHEEPINGROUP, -1, 0};
struct sembuf SignalProtectSheepInGroup={SEM_PSHEEPINGROUP, 1, 0};
//mealwait
struct sembuf WaitProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, 1, 0};

/*Number waiting sempahores*/
//cow
struct sembuf WaitCowsWaiting={SEM_COWSWAITING, -1, 0};
struct sembuf SignalCowsWaiting={SEM_COWSWAITING, 1, 0};
//sheep
struct sembuf WaitSheepWaiting={SEM_SHEEPWAITING, -1, 0};
struct sembuf SignalSheepWaiting={SEM_SHEEPWAITING, 1, 0};

/*Number eaten or fought semaphores*/
//cow
struct sembuf WaitCowsEaten={SEM_COWSEATEN, -1, 0};
struct sembuf SignalCowsEaten={SEM_COWSEATEN, 1, 0};
//sheep
struct sembuf WaitSheepEaten={SEM_SHEEPEATEN, -1, 0};
struct sembuf SignalSheepEaten={SEM_SHEEPEATEN, 1, 0};

/*Number eaten or fought mutexes*/
//cow
struct sembuf WaitProtectCowsEaten={SEM_PCOWSEATEN, -1, 0};
struct sembuf SignalProtectCowsEaten={SEM_PCOWSEATEN, 1, 0};
//sheep
struct sembuf WaitProtectSheepEaten={SEM_PSHEEPEATEN, -1, 0};
struct sembuf SignalProtectSheepEaten={SEM_PSHEEPEATEN, 1, 0};

/*Number Dead semaphores*/
//cow
struct sembuf WaitCowsDead={SEM_COWSDEAD, -1, 0};
struct sembuf SignalCowsDead={SEM_COWSDEAD, 1, 0};
//sheep
struct sembuf WaitSheepDead={SEM_SHEEPDEAD, -1, 0};
struct sembuf SignalSheepDead={SEM_SHEEPDEAD, 1, 0};

/*Dragon Semaphores*/
struct sembuf WaitDragonEating={SEM_DRAGONEATING, -1, 0};
struct sembuf SignalDragonEating={SEM_DRAGONEATING, 1, 0};
struct sembuf WaitDragonFighting={SEM_DRAGONFIGHTING, -1, 0};
struct sembuf SignalDragonFighting={SEM_DRAGONFIGHTING, 1, 0};
struct sembuf WaitDragonSleeping={SEM_DRAGONSLEEPING, -1, 0};
struct sembuf SignalDragonSleeping={SEM_DRAGONSLEEPING, 1, 0};
struct sembuf WaitDragonPlaying={SEM_DRAGONPLAYING, -1, 0};
struct sembuf SignalDragonPlaying={SEM_DRAGONPLAYING, 1, 0};

/*Termination Mutex*/
struct sembuf WaitProtectTerminate={SEM_PTERMINATE, -1, 0};
struct sembuf SignalProtectTerminate={SEM_PTERMINATE, 1, 0};
struct sembuf WaitProtectFightResults={SEM_PFIGHTRESULTS, -1, 0};
struct sembuf SignalProtectFightResults={SEM_PFIGHTRESULTS, 1, 0};

//theif
struct sembuf WaitThiefWaiting={SEM_THIEFWAITING, -1, 0};
struct sembuf SignalThiefWaiting={SEM_THIEFWAITING, 1, 0};
struct sembuf WaitPlayOver={SEM_PLAYOVER, -1, 0};
struct sembuf SignalPlayOver={SEM_PLAYOVER, 1, 0};
struct sembuf WaitThiefDefeated={SEM_THIEFDEFEATED, -1, 0};
struct sembuf SignalThiefDefeated={SEM_THIEFDEFEATED, 1, 0};
struct sembuf WaitProtectThiefWaiting={SEM_PTHIEFWAITING, -1, 0};
struct sembuf SignalProtectThiefWaiting={SEM_PTHIEFWAITING, 1, 0};
struct sembuf WaitProtectThiefWaitingFlag={SEM_PTHIEFWAITINGFLAG, -1, 0};
struct sembuf SignalProtectThiefWaitingFlag={SEM_PTHIEFWAITINGFLAG, 1, 0};
struct sembuf WaitThiefPlaying={SEM_THIEFPLAYING, -1, 0};
struct sembuf SignalThiefPlaying={SEM_THIEFPLAYING, 1, 0};
struct sembuf WaitProtectThiefDefeated={SEM_PTHIEFDEFEATED, -1, 0};
struct sembuf SignalProtectThiefDefeated={SEM_PTHIEFDEFEATED, 1, 0};

//hunter
struct sembuf WaitHuntersWaiting={SEM_HUNTERSWAITING, -1, 0};
struct sembuf SignalHuntersWaiting={SEM_HUNTERSWAITING, 1, 0};
struct sembuf WaitHuntersDefeated={SEM_HUNTERSDEFEATED, -1, 0};
struct sembuf SignalHuntersDefeated={SEM_HUNTERSDEFEATED, 1, 0};
struct sembuf WaitProtectHuntersWaiting={SEM_PHUNTERSWAITING, -1, 0};
struct sembuf SignalProtectHuntersWaiting={SEM_PHUNTERSWAITING, 1, 0};
struct sembuf WaitHunterFighting={SEM_HUNTERFIGHTING, -1, 0};
struct sembuf SignalHunterFighting={SEM_HUNTERFIGHTING, 1, 0};
struct sembuf WaitProtectHuntersDefeated={SEM_PHUNTERSDEFEATED, -1, 0};
struct sembuf SignalProtectHuntersDefeated={SEM_PHUNTERSDEFEATED, 1, 0};
struct sembuf WaitFightOver={SEM_FIGHTOVER, -1, 0};
struct sembuf SignalFightOver={SEM_FIGHTOVER, 1, 0};

double timeChange( struct timeval starttime );
void initialize();
void terminateSimulation();
void releaseSemandMem();
void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something);
void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo);
void cow(int startTimeN);
void sheep(int startTimeN);
void thief(int startTimeN);
void hunter(int startTimeN);
void smaug();

void initialize()
{
	/* Init semaphores */
	semID=semget(IPC_PRIVATE, 50, 0666 | IPC_CREAT);


	/* Init to zero, no elements are produced yet */
	seminfo.val=0;
	semctlChecked(semID, SEM_COWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_THIEFWAITING, SETVAL, seminfo);

	semctlChecked(semID, SEM_DRAGONPLAYING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_PLAYOVER, SETVAL, seminfo);
	semctlChecked(semID, SEM_FIGHTOVER, SETVAL, seminfo);

	semctlChecked(semID, SEM_SHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_DRAGONFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONEATING, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");

	/* Init Mutex to one */
	seminfo.val=1;
	semctlChecked(semID, SEM_PCOWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PCOWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_PTERMINATE, SETVAL, seminfo);

	semctlChecked(semID, SEM_PTHIEFWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_THIEFPLAYING, SETVAL, seminfo);
	semctlChecked(semID, SEM_THIEFDEFEATED, SETVAL, seminfo);
	semctlChecked(semID, SEM_PFIGHTRESULTS, SETVAL, seminfo);
	semctlChecked(semID, SEM_PHUNTERSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERSDEFEATED, SETVAL, seminfo);
	semctlChecked(semID, SEM_PTHIEFWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PHUNTERWAITINGFLAG, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");


	/* Now we create and attach  the segments of shared memory*/
	if ((terminateFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
	{
               printf("!!INIT!!INIT!!INIT!!  shm not created for terminateFlag\n");
	       exit(1);
	}
	else
	{
	       printf("!!INIT!!INIT!!INIT!!  shm created for terminateFlag\n");
	}

	if ((cowCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
	if ((sheepCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepCounter\n");
	}

	if ((mealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((cowsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowsEatenCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowsEatenCounter\n");
	}
	if ((defeatedThiefCount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for defeatedThiefCount\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for defeatedThiefCount\n");
	}
	if ((defeatedHunterCount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for defeatedHunterCount\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for defeatedHunterCount\n");
	}


	if ((sheepEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepEatenCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepEatenCounter\n");
	}
	if ((hunterWaiting = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for hunterWaiting\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for hunterWaiting\n");
	}
	if ((thiefWaiting = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for thiefWaiting\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for thiefWaiting\n");
	}
	if ((fightOutcome = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for fightOutcome\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
	if ((winProb = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not created for winProb\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm created for winProb\n");
	}


	/* Now we attach the segment to our data space.  */
        if ((terminateFlagp = shmat(terminateFlag, NULL, 0)) == (int *) -1)
        {
                printf("!!INIT!!INIT!!INIT!!  shm not attached for terminateFlag\n");
                exit(1);
        }
        else
        {
                 printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
        }
        if ((winProbp = shmat(winProb, NULL, 0)) == (int *) -1)
        {
                printf("!!INIT!!INIT!!INIT!!  shm not attached for winProb\n");
                exit(1);
        }
        else
        {
                 printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
        }

	if ((cowCounterp = shmat(cowCounter, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
	if ((fightOutcomep = shmat(fightOutcome, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}

	if ((sheepCounterp = shmat(sheepCounter, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepCounter\n");
	}

	if ((mealWaitingFlagp = shmat(mealWaitingFlag, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
	if ((cowsEatenCounterp = shmat(cowsEatenCounter, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowsEatenCounter\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowsEatenCounter\n");
	}
	if ((sheepEatenCounterp = shmat(sheepEatenCounter, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepEatenCounter\n");
	}

	if ((defeatedThiefCountp = shmat(defeatedThiefCount, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for defeatedThiefCount\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for defeatedThiefCount\n");
	}
	if ((defeatedHunterCountp = shmat(defeatedHunterCount, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for defeatedHunterCount\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for defeatedHunterCount\n");
	}

	if ((thiefWaitingp = shmat(thiefWaiting, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for thiefWaiting\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for thiefWaiting\n");
	}
	if ((hunterWaitingp = shmat(hunterWaiting, NULL, 0)) == (int *) -1)
        {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for hunterWaiting\n");
		exit(1);
	}
	else
        {
		printf("!!INIT!!INIT!!INIT!!  shm attached for hunterWaiting\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}

void releaseSemandMem()
{
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();

	//should check return values for clean termination
	semctl(semID, 0, IPC_RMID, seminfo);


	// wait for the semaphores
	usleep(2000);
	while((w = waitpid( -1, &status, WNOHANG)) > 1)
        {
			printf("                           REAPED process in terminate %d\n", w);
	}
	printf("\n");
        if(shmdt(terminateFlagp)==-1)
        {
                printf("RELEASERELEASERELEAS   terminateFlag share memory detach failed\n");
        }
        else
        {
                printf("RELEASERELEASERELEAS   terminateFlag share memory detached\n");
        }
        if( shmctl(terminateFlag, IPC_RMID, NULL ))
        {
                printf("RELEASERELEASERELEAS   share memory delete failed %d\n",*terminateFlagp );
        }
        else
        {
                printf("RELEASERELEASERELEAS   share memory deleted\n");
        }
	if( shmdt(cowCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowCounterp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   cowCounterp memory detached\n");
	}
	if( shmctl(cowCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowCounter memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   cowCounter memory deleted\n");
	}

	//sheep
	if( shmdt(sheepCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   sheepCounterp memory detached\n");
	}
	if( shmctl(sheepCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepCounter memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   sheepCounter memory deleted\n");
	}

	if( shmdt(mealWaitingFlagp)==-1)
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detached\n");
	}
	if( shmctl(mealWaitingFlag, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory deleted\n");
	}
	//cow
	if( shmdt(cowsEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detached\n");
	}
	if( shmctl(cowsEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory deleted\n");
	}

	//sheep
	if( shmdt(sheepEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detached\n");
	}
	if( shmctl(sheepEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory deleted\n");
	}

	//thief
	if( shmdt(thiefWaitingp)==-1)
	{
		printf("RELEASERELEASERELEAS   thiefWaitingp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   thiefWaitingp memory detached\n");
	}
	if( shmctl(thiefWaiting, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   thiefWaiting memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   thiefWaiting memory deleted\n");
	}


	if( shmdt(defeatedThiefCountp)==-1)
	{
		printf("RELEASERELEASERELEAS   defeatedThiefCountp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   defeatedThiefCountp memory detached\n");
	}
	if( shmctl(defeatedThiefCount, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   defeatedThiefCount memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   defeatedThiefCount memory deleted\n");
	}


	if( shmdt(winProbp)==-1)
	{
		printf("RELEASERELEASERELEAS   winProbp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   winProbp memory detached\n");
	}
	if( shmctl(winProb, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   winProb memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   winProb memory deleted\n");
	}

	//hunter
	if( shmdt(hunterWaitingp)==-1)
	{
		printf("RELEASERELEASERELEAS   hunterWaitingp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   hunterWaitingp memory detached\n");
	}
	if( shmctl(hunterWaiting, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   hunterWaiting memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   hunterWaiting memory deleted\n");
	}

	if( shmdt(defeatedHunterCountp)==-1)
	{
		printf("RELEASERELEASERELEAS   defeatedHunterCountp memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   defeatedHunterCountp memory detached\n");
	}
	if( shmctl(defeatedHunterCount, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   defeatedHunterCount memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   defeatedHunterCount memory deleted\n");
	}

	if( shmdt(fightOutcomep)==-1)
	{
		printf("RELEASERELEASERELEAS   fightOutcomep memory detach failed\n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   fightOutcomep memory detached\n");
	}
	if( shmctl(fightOutcome, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   fightOutcome memory delete failed \n");
	}
	else
        {
		printf("RELEASERELEASERELEAS   fightOutcome memory deleted\n");
	}

}


void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo)
{
	/* wrapper that checks if the semaphore control request has terminated */
	/* successfully. If it has not the entire simulation is terminated */

	if (semctl(semaphoreID, semNum, flag,  seminfo) == -1 )
        {
		if(errno != EIDRM)
                {
			printf("semaphore control failed: simulation terminating\n");
			printf("errno %8d \n",errno );
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else
                {
			exit(3);
		}
	}
}

void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something)
{

	/* wrapper that checks if the semaphore operation request has terminated */
	/* successfully. If it has not the entire simulation is terminated */
	if (semop(semaphoreID, operation, something) == -1 )
        {
		if(errno != EIDRM)
                {
			printf("semaphore operation failed: simulation terminating\n");
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else
                {
			exit(3);
		}
	}
}

double timeChange( const struct timeval startTime )
{
	struct timeval nowTime;
	double elapsedTime;

	gettimeofday(&nowTime,NULL);
	elapsedTime = (nowTime.tv_sec - startTime.tv_sec)*1000.0;
	elapsedTime = elapsedTime + (nowTime.tv_usec - startTime.tv_usec)/1000.0;
	return elapsedTime;

}

int rand_lim(int limit)
{
/* return 1 if wins and 0 is loses.
 */
    int retval;

    retval = rand() %100;
    if(retval < limit)
    {
    	return 1;
    }
    else
    {
        return 0;
    }
}

void terminateSimulation()
{
	pid_t localpgid;
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();
	printf("RELEASESEMAPHORES   Terminating Simulation from process %8d\n", localpgid);
	if(cowProcessGID != (int)localpgid )
        {
		if(killpg(cowProcessGID, SIGKILL) == -1 && errno == EPERM)
                {
			printf("XXTERMINATETERMINATE   COWS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed cows \n");
	}
	if(sheepProcessGID != (int)localpgid )
        {
		if(killpg(sheepProcessGID, SIGKILL) == -1 && errno == EPERM)
                {
			printf("XXTERMINATETERMINATE   SHEEP NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed SHEEP \n");
	}
	if(thiefProcessGID != (int)localpgid )
        {
		if(killpg(thiefProcessGID, SIGKILL) == -1 && errno == EPERM)
                {
			printf("XXTERMINATETERMINATE   THIEF NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed THIEF \n");
	}
	if(hunterProcessGID != (int)localpgid )
        {
		if(killpg(hunterProcessGID, SIGKILL) == -1 && errno == EPERM)
                {
			printf("XXTERMINATETERMINATE   HUNTER NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed HUNTER \n");
	}
	if(smaugProcessID != (int)localpgid )
        {
		kill(smaugProcessID, SIGKILL);
		printf("XXTERMINATETERMINATE   killed smaug\n");
	}
	while( (w = waitpid( -1, &status, WNOHANG)) > 1)
        {
			printf("                           REAPED process in terminate %d\n", w);
	}
	releaseSemandMem();
	printf("GOODBYE from terminate\n");
}




void smaug()
{
	int k;
	int temp;
	int localpid;
	double elapsedTime;

	/* local counters used only for smaug routine */
	int cowsEatenTotal = 0;
	int sheepEatenTotal = 0;
	int thiefPlayed = 0;
	int hunterFought = 0;
	int treasure = INITIAL_TREASURE_IN_HOARD;


	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	smaugProcessID = getpid();
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   PID is %d \n", smaugProcessID );
	localpid = smaugProcessID;
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has gone to sleep\n" );
	semopChecked(semID, &WaitDragonSleeping, 1);
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has woken up \n" );
	while (TRUE)
        {
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		if(*mealWaitingFlagp >=1)
                {
			*mealWaitingFlagp = *mealWaitingFlagp - 1;
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   signal meal flag %d\n", *mealWaitingFlagp);
			semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is eating a meal\n");
			for( k = 0; k < COWS_IN_GROUP; k++ )
                        {
				semopChecked(semID, &SignalCowsWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A cow is ready to eat\n");
			}
			for(k = 0; k < SHEEP_IN_GROUP; k++)
                        {
				semopChecked(semID, &SignalSheepWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A sheep is ready to eat\n");
			}
			/*Smaug waits to eat*/
			semopChecked(semID, &WaitDragonEating, 1);
			for( k = 0; k < COWS_IN_GROUP; k++ )
                        {
				semopChecked(semID, &SignalCowsDead, 1);
				cowsEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a cow\n");
			}
			for(k = 0; k < SHEEP_IN_GROUP;k++)
                        {
				semopChecked(semID, &SignalSheepDead, 1);
				sheepEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a sheep\n");
			}

			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a meal\n");
			if(cowsEatenTotal >= MAX_COWS_EATEN )
                        {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of cows\n");
				*terminateFlagp= 1;
				break;
			}
			if(sheepEatenTotal >= MAX_SHEEP_EATEN )
                        {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of sheep\n");
				*terminateFlagp= 1;
				break;
			}

			/* Smaug check to see if another snack is waiting */
			semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
			if( *mealWaitingFlagp > 0  )
                        {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug eats again\n");
				semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			}
                        else
                        {
				semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n");
				semopChecked(semID, &WaitDragonSleeping, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n");
			}
		}
                else
                {
			semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
			if(*thiefWaitingp >= 1 && *mealWaitingFlagp <= 0 )
                        {

			/* decrement the number of thieves blocked */
		 	/* by the semaphore */
                 		*thiefWaitingp = *thiefWaitingp - 1;
                 		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
                  	 	semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
                 	 	semopChecked(semID, &SignalThiefWaiting, 1);
              	 		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug waiting to play with thief\n");
		 	 	/* wait till the thief is ready to play */
                 	 	semopChecked(semID, &WaitDragonPlaying, 1);
                   		thiefPlayed++;
                   		semopChecked(semID, &WaitPlayOver, 1);
                   	 	semopChecked(semID, &WaitProtectFightResults, 1);
		   	 	/* after playing pay the price */
		   	 	if(*fightOutcomep == 0)
                                {
		       			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug wins 20 gems\n");
		    	  		treasure = treasure + 20;
		    	  		*defeatedThiefCountp = *defeatedThiefCountp + 1;
		    		}
		    		else
                                {
		    			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug loses 8 gems\n");
		     	 		treasure = treasure - 8;
		    		}
		    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has %d treasure\n",treasure);
		    		semopChecked(semID, &SignalProtectFightResults, 1);
                    		/* release the thief from the game */
                   	 	semopChecked(semID, &SignalThiefDefeated, 1);

		    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated %d thieves\n", *defeatedThiefCountp);
		   		if(*defeatedThiefCountp >= MAX_THIEVES_DEFEATED)
                                {
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated the max amount of thieves\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}
		   		if(treasure >= MAX_TREASURE_IN_HOARD)
                                {
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has more than 800 jewels\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}
		   		if(treasure <= 0)
                                {
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has no more treasure\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}

                                //check if there is another thief
                                semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
                                if(*thiefWaitingp >= 1 && *mealWaitingFlagp <= 0 )
                                {

					/* decrement the number of thieves blocked */
				 	/* by the semaphore */
		         		*thiefWaitingp = *thiefWaitingp - 1;
		         		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
		          	 	semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		         	 	semopChecked(semID, &SignalThiefWaiting, 1);
		      	 		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug waiting to play with thief\n");
			 	 	/* wait till the thief is ready to play */
		         	 	semopChecked(semID, &WaitDragonPlaying, 1);
		           		thiefPlayed++;
		           		semopChecked(semID, &WaitPlayOver, 1);
		           	 	semopChecked(semID, &WaitProtectFightResults, 1);
			   	 	/* after playing pay the price */
			   	 	if(*fightOutcomep == 0)
                                        {
			       			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug wins 20 gems\n");
			    	  		treasure = treasure + 20;
			    	  		*defeatedThiefCountp = *defeatedThiefCountp + 1;
			    		}
			    		else
                                        {
			    			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug loses 8 gems\n");
			     	 		treasure = treasure - 8;
			    		}
			    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has %d treasure\n",treasure);
			    		semopChecked(semID, &SignalProtectFightResults, 1);
		            		/* release the thief from the game */
		           	 	semopChecked(semID, &SignalThiefDefeated, 1);

			    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated %d thieves\n", *defeatedThiefCountp);
			   		if(*defeatedThiefCountp >= MAX_THIEVES_DEFEATED)
                                        {
			   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated the max amount of thieves\n");
			    			*terminateFlagp= 1;
			    			break;
			   		}
			   		if(treasure >= MAX_TREASURE_IN_HOARD)
                                        {
			   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has more than 800 jewels\n");
			    			*terminateFlagp= 1;
			    			break;
			   		}
			   		if(treasure <= 0)
                                        {
			   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has no more treasure\n");
			    			*terminateFlagp= 1;
			    			break;
			   		}
                                        printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n");
					semopChecked(semID, &WaitDragonSleeping, 1);
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n");
				 }
                                 else
                                 {
			 		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			 		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
                                        printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n");
					semopChecked(semID, &WaitDragonSleeping, 1);
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n");
		 	         }
			 }
                         else
                         {
		 		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		 		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
		 	 }




		 	semopChecked(semID, &WaitProtectHuntersWaiting, 1);
		 	semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
		 	semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
			if( *hunterWaitingp >= 1 && *thiefWaitingp <=0 && *mealWaitingFlagp <= 0)
                        {

			/* decrement the number of hunters blocked */
		 	/* by the semaphore */
                 		*hunterWaitingp = *hunterWaitingp - 1;
                 		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
                  	 	semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
                  	 	semopChecked(semID, &SignalProtectHuntersWaiting, 1);

                 	 	semopChecked(semID, &SignalHuntersWaiting, 1);
              	 		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug waiting to fight with hunter\n");
		 	 	/* wait till the hunter is ready to fight */
                 	 	semopChecked(semID, &WaitDragonFighting, 1);
                   		hunterFought++;
                   		semopChecked(semID, &WaitFightOver, 1);
                   	 	semopChecked(semID, &WaitProtectFightResults, 1);
		   	 	/* after playing pay the price */
		   	 	if(*fightOutcomep == 0)
                                {
		       			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug wins 5 jewels\n");
		    	  		treasure = treasure + 5;
		    	  		*defeatedHunterCountp = *defeatedHunterCountp + 1;
		    		}
		    		else
                                {
		    			printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug loses 10 gems\n");
		     	 		treasure = treasure - 10;
		    		}
		    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has %d treasure\n",treasure);
		    		semopChecked(semID, &SignalProtectFightResults, 1);
                    		/* release the hunter from the game */
                   	 	semopChecked(semID, &SignalHuntersDefeated, 1);

		    		printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated %d hunters\n", *defeatedHunterCountp);
		   		if(*defeatedHunterCountp >= MAX_HUNTER_DEFEATED)
                                {
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has defeated the max amount of hunters\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}
		   		if(treasure >= MAX_TREASURE_IN_HOARD)
				{
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has more than 800 jewels\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}
		   		if(treasure <= 0)
                                {
		   		        printf("SMAUGSMAUGSMAUGSMAUGSMAU   smaug has no more treasure\n");
		    			*terminateFlagp= 1;
		    			break;
		   		}
			 }
                         else
                         {
		 		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		 		semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
		 		semopChecked(semID, &SignalProtectHuntersWaiting, 1);
		 	 }


                }
                usleep(1000);
	}
}





void cow(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("CCCCCCC %8d CCCCCCC   A cow is born\n", localpid);
	if( startTimeN > 0)
        {
		if( usleep( startTimeN) == -1)
                {
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)
                        {
                               exit(4);
                        }
		}
	}
	printf("CCCCCCC %8d CCCCCCC   cow grazes for %f ms\n", localpid, startTimeN/1000.0);

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsInGroup, 1);
	semopChecked(semID, &WaitProtectSheepInGroup, 1);
	semopChecked(semID, &SignalCowsInGroup, 1);
	*cowCounterp = *cowCounterp + 1;
	printf("CCCCCCC %8d CCCCCCC   %d  cows have been enchanted \n", localpid, *cowCounterp );
	if((*cowCounterp  >= COWS_IN_GROUP && *sheepCounterp >= SHEEP_IN_GROUP)) {
		*cowCounterp = *cowCounterp - COWS_IN_GROUP;
		*sheepCounterp = *sheepCounterp - SHEEP_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++)
                {
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		for(k = 0; k < SHEEP_IN_GROUP;k++)
                {
			semopChecked(semID, &WaitSheepInGroup, 1);
		}

		printf("CCCCCCC %8d CCCCCCC   The last cow is waiting\n", localpid);
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		printf("CCCCCCC %8d CCCCCCC   signal meal flag %d\n", localpid, *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		semopChecked(semID, &SignalDragonSleeping, 1);
		printf("CCCCCCC %8d CCCCCCC   last cow  wakes the dragon \n", localpid);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
	}

	semopChecked(semID, &WaitCowsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsEaten, 1);
	semopChecked(semID, &SignalCowsEaten, 1);
	*cowsEatenCounterp = *cowsEatenCounterp + 1;
	if((*cowsEatenCounterp >= COWS_IN_GROUP))
        {
		*cowsEatenCounterp = *cowsEatenCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++)
                {
       		        semopChecked(semID, &WaitCowsEaten, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		printf("CCCCCCC %8d CCCCCCC   A cow is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitCowsDead, 1);

	printf("CCCCCCC %8d CCCCCCC   cow  dies\n", localpid);
}

void sheep(int startTimeN)
{
	int localpid;
	localpid = getpid();
	int k;

	/* graze */
	printf("SSSSSSS %8d SSSSSSS   A sheep is born\n", localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}
	}
	printf("SSSSSSS %8d SSSSSSS   sheep grazes for %f ms\n", localpid, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepInGroup, 1);
	semopChecked(semID, &SignalSheepInGroup, 1);
	*sheepCounterp = *sheepCounterp + 1;
	printf("SSSSSSS %8d SSSSSSS   %d  Sheep have been enchanted \n", localpid, *sheepCounterp );

	semopChecked(semID, &SignalProtectSheepInGroup, 1);

	semopChecked(semID, &WaitProtectSheepEaten, 1);
	semopChecked(semID, &SignalSheepEaten, 1);
	*sheepEatenCounterp = *sheepEatenCounterp + 1;
	if((*sheepEatenCounterp >= SHEEP_IN_GROUP))
        {
		*sheepEatenCounterp = *sheepEatenCounterp - SHEEP_IN_GROUP;
		for (k=0; k < SHEEP_IN_GROUP; k++)
                {
       		        semopChecked(semID, &WaitSheepEaten, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last Sheep has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectSheepEaten, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		printf("SSSSSSS %8d SSSSSSS   A Sheep is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitSheepWaiting, 1);
	semopChecked(semID, &WaitSheepDead, 1);
	printf("SSSSSSS %8d SSSSSSS   Sheep  dies\n", localpid);
}

void thief(int startTimeN)
{
	int localpid;
	localpid = getpid();

	printf("TTTTTTT %8d TTTTTTT   A thief is born\n", localpid);
	if(startTimeN > 0)
        {
		if(usleep(startTimeN) == -1)
                {
			if(errno==EINTR)
                        {
                              exit(4);
                        }
		}
	}
	printf("TTTTTTT %8d TTTTTTT   thief travels for %f ms\n", localpid, startTimeN/1000.0);

	semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
	*thiefWaitingp = *thiefWaitingp+ 1;
	semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);

	semopChecked(semID, &SignalDragonSleeping, 1);

	printf("TTTTTTT %8d TTTTTTT   A thief is waiting\n", localpid);
	semopChecked(semID, &WaitThiefWaiting, 1);
	semopChecked(semID, &SignalDragonPlaying, 1);
	printf("TTTTTTT %8d TTTTTTT   A thief is playing with smaug\n", localpid);
	semopChecked(semID, &WaitProtectFightResults, 1);
	*fightOutcomep = rand_lim(*winProbp);
	if(*fightOutcomep == 1)
        {
		printf("TTTTTTT %8d TTTTTTT   thief win 8 gems \n", localpid);
	}
	else
        {
		printf("TTTTTTT %8d TTTTTTT   thief lose 20 gems \n", localpid);
	}
	semopChecked(semID, &SignalProtectFightResults, 1);
	semopChecked(semID, &SignalPlayOver, 1);
	printf("TTTTTTT %8d TTTTTTT   thief has left the cave\n", localpid);
}

void hunter(int startTimeN)
{
	int localpid;
	localpid = getpid();

	printf("HHHHHHH %8d HHHHHHH   A hunter is born\n", localpid);
	if(startTimeN > 0)
        {
		if(usleep( startTimeN) == -1)
                {
			if(errno==EINTR)exit(4);
		}
	}
	printf("HHHHHHH %8d HHHHHHH   hunter travels for %f ms\n", localpid, startTimeN/1000.0);

	semopChecked(semID, &WaitProtectHuntersWaiting, 1);
	*hunterWaitingp = *hunterWaitingp+ 1;
	semopChecked(semID, &SignalProtectHuntersWaiting, 1);

	semopChecked(semID, &SignalDragonSleeping, 1);

	printf("HHHHHHH %8d HHHHHHH   A hunter is waiting\n", localpid);
	semopChecked(semID, &WaitHuntersWaiting, 1);
	semopChecked(semID, &SignalDragonFighting, 1);
	printf("HHHHHHH %8d HHHHHHH   A hunter is fighting with smaug\n", localpid);
	semopChecked(semID, &WaitProtectFightResults, 1);
	*fightOutcomep = rand_lim(*winProbp);
	if(*fightOutcomep == 1)
        {
		printf("HHHHHHH %8d HHHHHHH   hunter win 10 gems \n", localpid);
	}
	else
        {
		printf("HHHHHHH %8d HHHHHHH   hunter lose 5 jewels \n", localpid);
	}
	semopChecked(semID, &SignalProtectFightResults, 1);
	semopChecked(semID, &SignalFightOver, 1);
	printf("HHHHHHH %8d HHHHHHH   hunter has left the cave\n", localpid);
}






int main( )
{
	int k;
	int temp;
	int parentPID = 0;
	int cowPID = 0;
	int sheepPID = 0;
	int smaugPID = 0;
	int thiefPID = 0;
	int hunterPID = 0;
	int cowsCreated = 0;
	int sheepCreated = 0;
	int thiefCreated = 0;
	int hunterCreated = 0;
	int newSeed = 0;
	int sleepingTime = 0;
	int maxCowIntervalUsec = 0;
	int maxSheepIntervalUsec = 0;
	int maxThiefIntervalUsec = 0;
	int maxHunterIntervalUsec = 0;
	int nextInterval = 0.0;
	int status;
	int w = 0;
        double totalCowInterval = 0.0;
	double totalSheepInterval = 0.0;
	double totalThiefInterval = 0.0;
	double totalHunterInterval = 0.0;
        double maxCowInterval = 0.0;
	double maxSheepInterval = 0.0;
	double maxThiefInterval = 0.0;
	double maxHunterInterval = 0.0;
	double elapsedTime;
	int winProb2 = 0;

	parentPID = getpid();
	setpgid(parentPID, parentPID);
	parentProcessGID = getpgid(0);
	printf("CRCRCRCRCRCRCRCRCRCRCRCR  main process group  %d %d\n", parentPID, parentProcessGID);

	initialize();

	*cowCounterp = 0;
	*cowsEatenCounterp = 0;

	*sheepCounterp = 0;
	*sheepEatenCounterp = 0;
	*mealWaitingFlagp = 0;

	printf("Please enter a random seed to start the simulation\n");
	scanf("%d",&newSeed);
	srand(newSeed);

	printf("Please enter the maximum interval length for cow (ms)");
	scanf("%lf", &maxCowInterval);
	maxCowIntervalUsec = (int)maxCowInterval * 1000;
	printf("max Cow interval time %f \n", maxCowInterval);

	printf("Please enter the maximum interval length for sheep (ms)");
	scanf("%lf", &maxSheepInterval);
	maxSheepIntervalUsec = (int)maxSheepInterval * 1000;
	printf("max Sheep interval time %f \n", maxSheepInterval);

	printf("Please enter the maximum interval length for hunter (ms)");
	scanf("%lf", &maxHunterInterval);
	maxHunterIntervalUsec = (int)maxHunterInterval * 1000;
	printf("max Hunter interval time %f \n", maxHunterInterval);

	printf("Please enter the maximum interval length for thief (ms)");
	scanf("%lf", &maxThiefInterval);
	maxThiefIntervalUsec = (int)maxThiefInterval * 1000;
	printf("max thief interval time %f \n", maxThiefInterval);

	printf("Please enter the visitor win probability (percent)");
	scanf("%d", &winProb2);
	*winProbp = winProb2;

	gettimeofday(&startTime,NULL);

	if ((smaugPID = fork())==0)
        {
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug is born\n");
		smaug();
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug dies\n");
		exit(0);
	}
	else
        {
		if(smaugProcessID == -1)
                {
                         smaugProcessID = smaugPID;
                }
		setpgid(smaugPID, smaugProcessID);
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID %8d PGID %8d\n", smaugPID, smaugProcessID);
	}

	printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID  create cow %8d \n", smaugPID);
        usleep(10);
        while( 1 )
        {
		semopChecked(semID, &WaitProtectTerminate, 1);
       		if(*terminateFlagp != 0)
                {
			semopChecked(semID, &SignalProtectTerminate, 1);
			break;
		}
		semopChecked(semID, &SignalProtectTerminate, 1);

                //cow
		elapsedTime = timeChange(startTime);
		if( totalCowInterval - elapsedTime < totalCowInterval)
                {
			nextInterval=(int)((double)rand() / RAND_MAX * maxCowIntervalUsec);
			totalCowInterval = totalCowInterval + nextInterval/1000.0;
			sleepingTime = (int)( (double)rand() / RAND_MAX * maxCowIntervalUsec);
			if ((cowPID = fork())==0)
                        {
				elapsedTime = timeChange(startTime);
				cow( sleepingTime );
				exit(0);
			}
			else if( cowPID > 0)
                        {
				cowsCreated++;
				if(cowProcessGID == -1)
                                {
					cowProcessGID = cowPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  cow PGID %8d \n", cowPID, cowProcessGID);
				}
				setpgid(cowPID, cowProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   New cow created %8d \n", cowsCreated);
			}
			else
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			if(cowsCreated == MAX_COWS_CREATED)
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of cows\n");
				*terminateFlagp = 1;
				break;
			}
	        usleep(totalCowInterval*800);
		}

		//sheep
		elapsedTime = timeChange(startTime);
		if(totalSheepInterval - elapsedTime < totalSheepInterval)
                {
			nextInterval =(int)((double)rand() / RAND_MAX * maxSheepIntervalUsec);
			totalSheepInterval = totalSheepInterval + nextInterval/1000.0;
			sleepingTime = (int)((double)rand()/ RAND_MAX * maxSheepIntervalUsec);
			if ((sheepPID = fork())==0)
                        {
				elapsedTime = timeChange(startTime);
				sheep(sleepingTime);
				exit(0);
			}
			else if(sheepPID > 0)
                        {
				sheepCreated++;
				if(sheepProcessGID == -1)
                                {
					sheepProcessGID = sheepPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  sheep PGID %8d \n", sheepPID, sheepProcessGID);
				}
				setpgid(sheepPID,sheepProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   New sheep created %8d \n", sheepCreated);
			}
			else
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			if(sheepCreated == MAX_SHEEP_CREATED)
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of sheep\n");
				*terminateFlagp = 1;
				break;
			}
	        usleep(totalSheepInterval*800);
		}

		//thief
		if(totalThiefInterval - elapsedTime < totalThiefInterval)
                {
			nextInterval=(int)((double)rand() / RAND_MAX * maxThiefIntervalUsec);
			totalThiefInterval=totalThiefInterval+nextInterval/1000.0;
			sleepingTime=(int)((double)rand()/ RAND_MAX * maxThiefIntervalUsec);
			if((thiefPID=fork())==0)
                        {
				elapsedTime = timeChange(startTime);
				thief(sleepingTime);
				exit(0);
			}
			else if(thiefPID > 0)
                        {
				thiefCreated++;
				if(thiefProcessGID == -1)
                                {
					thiefProcessGID = thiefPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  Thief PGID %8d \n", thiefPID, thiefProcessGID);
				}
				setpgid(thiefPID, thiefProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   New Thief created %8d \n", thiefCreated);
			}
			else
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR thief process not created \n");
				continue;
			}
	        usleep(totalThiefInterval*800);
		}

		//hunter
		elapsedTime=timeChange(startTime);
		if(totalHunterInterval - elapsedTime < totalHunterInterval)
                {
			nextInterval=(int)((double)rand() / RAND_MAX * maxHunterIntervalUsec);
			totalHunterInterval=totalHunterInterval+nextInterval/1000.0;
			sleepingTime=(int)((double)rand()/ RAND_MAX * maxHunterIntervalUsec);
			if ((hunterPID=fork())==0)
                        {
				elapsedTime=timeChange(startTime);
				hunter(sleepingTime);
				exit(0);
			}
			else if(hunterPID > 0)
                        {
				hunterCreated++;
				if(hunterProcessGID == -1)
                                {
					hunterProcessGID=hunterPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  Hunter PGID %8d \n", hunterPID, hunterProcessGID);
				}
				setpgid(hunterPID, hunterProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   New hunter created %8d \n", hunterCreated);
			}
			else
                        {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR hunter process not created \n");
				continue;
			}
	        usleep(totalHunterInterval*800);
		}


		while((w=waitpid(-1,&status,WNOHANG))>1)
                {
		       	if (WIFEXITED(status))
                        {
			       	if (WEXITSTATUS(status) > 0)
                                {
					printf("exited, status=%8d\n", WEXITSTATUS(status));
					terminateSimulation();
					printf("End main process %8d\n", getpid());
					exit(0);
				}
				else
                                {
					printf("                           REAPED process %8d\n", w);
				}
			}
		}

		if( *terminateFlagp == 1 )break;
	}
	if(*terminateFlagp == 1)
        {
		terminateSimulation();
	}
	printf("End main process %8d\n", getpid());
	exit(0);
}
