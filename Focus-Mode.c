#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define SESSION_OUTRO "\nFocus Mode complete. All distractions are now unblocked.\n"
#define FIRST_ROUND_INTRO "Entering Focus Mode. All distractions are blocked.\n"
#define NON_FIRST_ROUND_INTRO \
    "──────────────────────────────────────────────\n" \
    "             Back to Focus Mode.              \n" \
    "══════════════════════════════════════════════\n"

#define ADDITIONAL_ROUND_INTRO \
    "══════════════════════════════════════════════\n" \
    "                Focus Round %d                \n" \
    "──────────────────────────────────────────────\n"

#define DISTRACTIONS_INTRO \
    "──────────────────────────────────────────────\n" \
    "        Checking pending distractions...      \n" \
    "──────────────────────────────────────────────\n"
#define DISTRACTIONS_INTRO_EMPTY "No distractions reached you this round.\n"

#define GET_MESSAGE_PROMPT \
    "\nSimulate a distraction:\n" \
    "  1 = Email notification\n" \
    "  2 = Reminder to pick up delivery\n" \
    "  3 = Doorbell Ringing\n" \
    "  q = Quit\n" \
    ">> " \

#define SIGNAL_EMAIL                            SIGUSR1
#define SIGNAL_DELIVERY                         SIGUSR2
#define SIGNAL_DOORBELL                         SIGINT

#define MESSAGE_QUIT                            'q'
#define MESSAGE_EMAIL                           '1'
#define MESSAGE_EMAIL_TEXT                      " - Email notification is waiting.\n"
#define MESSAGE_EMAIL_OUTCOME                   "[Outcome:] The TA announced: Everyone get 100 on the exercise!\n"
#define MESSAGE_DELIVERY                        '2'
#define MESSAGE_DELIVERY_TEXT                   " - You have a reminder to pick up your delivery.\n"
#define MESSAGE_DELIVERY_OUTCOME                "[Outcome:] You picked it up just in time.\n"
#define MESSAGE_DOORBELL                        '3'
#define MESSAGE_DOORBELL_TEXT                   " - The doorbell is ringing.\n"
#define MESSAGE_DOORBELL_OUTCOME                "[Outcome:] Food delivery is here.\n"

typedef enum
{
    false,
    true
} boolean;

void SetSignalAction(int signal, void (*action)(int));
char GetMessage();
void PlayRound(int roundNumber, int duration);
void ProcessSignals(const int* singals, int signalsCount);
void ProcessSignalsV2(const int* singals, int signalsCount);
void HandleFocusMode(int numOfRounds, int duration);
void SendSignal(char message);
int FindNewSignal(const int* seenSignals, int seenSignalsCount, const int* relevantSignals, int relevantSignalsCount);
void ChangedBlockedSignals(int action, const int* signals, int signalsCount);

void HandleFocusMode(int numOfRounds, int duration)
{
    for (int i = 1; i <= numOfRounds; i++)
        PlayRound(i, duration);

    // for some reason it was printed this way in the example
    printf(NON_FIRST_ROUND_INTRO);
    printf(SESSION_OUTRO);
}

char GetMessage()
{
    char messageType = -1;

    printf(GET_MESSAGE_PROMPT);
    scanf(" %c", &messageType);

    return messageType;
}

void PlayRound(int roundNumber, int duration)
{
    const int relevantSignalsCount = 3;
    const int relevantSignals[] = { SIGNAL_EMAIL, SIGNAL_DELIVERY, SIGNAL_DOORBELL };

    int seenSignalsCount = 0;
    int* seenSignals = (int*)malloc(relevantSignalsCount * sizeof(int));
    if (seenSignals == NULL)
    {
        perror("malloc() error");
        exit(EXIT_FAILURE);
    }

    printf(roundNumber == 1 ? FIRST_ROUND_INTRO : NON_FIRST_ROUND_INTRO);
    printf(ADDITIONAL_ROUND_INTRO, roundNumber);

    /*
     * BLOCK RELEVANT SIGNALS
     */
    ChangedBlockedSignals(SIG_BLOCK, relevantSignals, relevantSignalsCount);




    /*
     * GENERATE seenSignals
     */
    for (int i = 0; i < duration; i++)
    {
        int newSignal = -1;



        int message = GetMessage();
        if (message == MESSAGE_QUIT)
            break;



        SendSignal(message);
        newSignal = FindNewSignal(seenSignals, seenSignalsCount, relevantSignals, relevantSignalsCount);
        if (newSignal != -1)
        {
            seenSignals[seenSignalsCount] = newSignal;
            seenSignalsCount++;
        }
    }



    /*
     * PROCESSING THE SIGNALS WHICH WERE SENT
     */
    printf(DISTRACTIONS_INTRO);
    if (seenSignalsCount == 0)
        printf(DISTRACTIONS_INTRO_EMPTY);

    ProcessSignalsV2(seenSignals, seenSignalsCount);



    /*
     * CLEANUP PENDING SIGNALS
     */



    /*
     * SET SIGNAL ACTION TO IGNORE
     */
    for (int i = 0; i < seenSignalsCount; i++)
        SetSignalAction(seenSignals[i], SIG_IGN);



    /*
     * UNBLOCK SIGNALS
     */
    ChangedBlockedSignals(SIG_UNBLOCK, relevantSignals, relevantSignalsCount);



    /*
     * SET SIGNAL ACTION TO DEFAULT
     */
    for (int i = 0; i < seenSignalsCount; i++)
        SetSignalAction(seenSignals[i], SIG_DFL);



    free(seenSignals);
}

/*
 * For some reason it seems like we're not supposed to process the sign als in the order that they were received.
 */
void ProcessSignals(const int* signals, int signalsCount)
{
    for (int i = 0; i < signalsCount; i++)
    {
        char* messageText = NULL;
        char* messageOutcome = NULL;

        switch (signals[i])
        {
            case SIGNAL_EMAIL:
                messageText = MESSAGE_EMAIL_TEXT;
                messageOutcome = MESSAGE_EMAIL_OUTCOME;
                break;
            case SIGNAL_DELIVERY:
                messageText = MESSAGE_DELIVERY_TEXT;
                messageOutcome = MESSAGE_DELIVERY_OUTCOME;
                break;
            case SIGNAL_DOORBELL:
                messageText = MESSAGE_DOORBELL_TEXT;
                messageOutcome = MESSAGE_DOORBELL_OUTCOME;
                break;
            default:
                fprintf(stderr, "Invalid argument error: '%d' is an invalid signal number\n", signals[i]);
                break;
        }

        printf("%s", messageText);
        printf("%s", messageOutcome);
    }
}

/*
 * For some reason it seems like we're not supposed to process the sign als in the order that they were received.
 */
void ProcessSignalsV2(const int* signals, int signalsCount)
{
    boolean seenEmail = false;
    boolean seenDelivery = false;
    boolean seenDoorbell = false;

    for (int i = 0; i < signalsCount; i++)
    {
        switch (signals[i])
        {
            case SIGNAL_EMAIL:
                seenEmail = true;
                break;
            case SIGNAL_DELIVERY:
                seenDelivery = true;
                break;
            case SIGNAL_DOORBELL:
                seenDoorbell = true;
                break;
            default:
                fprintf(stderr, "Invalid argument error: '%d' is an invalid signal number\n", signals[i]);
                break;
        }
    }
    if (seenEmail)
        printf("%s%s", MESSAGE_EMAIL_TEXT, MESSAGE_EMAIL_OUTCOME);
    if (seenDelivery)
        printf("%s%s", MESSAGE_DELIVERY_TEXT, MESSAGE_DELIVERY_OUTCOME);
    if (seenDoorbell)
        printf("%s%s", MESSAGE_DOORBELL_TEXT, MESSAGE_DOORBELL_OUTCOME);
}

void SendSignal(char message)
{
    int signalNumber = -1;
    switch (message)
    {
        case MESSAGE_EMAIL:
            signalNumber = SIGNAL_EMAIL;
            break;
        case MESSAGE_DELIVERY:
            signalNumber = SIGNAL_DELIVERY;
            break;
        case MESSAGE_DOORBELL:
            signalNumber = SIGNAL_DOORBELL;
            break;
        default:
            break;
    }

    if (signalNumber == -1)
    {
        fprintf(stderr, "Invalid argument error: '%c' is invalid\n", message);
        exit(EXIT_FAILURE);
    }

    if (raise(signalNumber) != 0)
    {
        perror("raise() error");
        exit(EXIT_FAILURE);
    }
}

int FindNewSignal(const int* seenSignals, int seenSignalsCount, const int* relevantSignals, int relevantSignalsCount)
{
    sigset_t pendingSignals;
    if (sigpending(&pendingSignals) != 0)
    {
        perror("sigpending() error");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < relevantSignalsCount; i++)
    {
        if (sigismember(&pendingSignals, relevantSignals[i]))
        {
            boolean isSignalSeen = false;
            for (int j = 0; j < seenSignalsCount; j++)
            {
                if (relevantSignals[i] == seenSignals[j])
                    isSignalSeen = true;
            }
            if (!isSignalSeen)
                return relevantSignals[i];
        }
    }

    return -1;
}

void ChangedBlockedSignals(int action, const int* signals, int signalsCount)
{
    sigset_t signalsMask;
    sigemptyset(&signalsMask);

    for (int i = 0; i < signalsCount; i++)
        sigaddset(&signalsMask, signals[i]);

    if (sigprocmask(action, &signalsMask, NULL) == -1)
    {
        perror("sigprocmask() error");
        exit(EXIT_FAILURE);
    }
}

void SetSignalAction(int signal, void (*action)(int))
{
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        act.sa_handler = SIG_IGN;

        if (sigaction(signal, &act, NULL) != 0)
        {
            perror("sigaction() error");
            exit(EXIT_FAILURE);
        }
}
