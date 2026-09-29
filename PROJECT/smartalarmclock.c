#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ITEMS 200
#define TITLE_LEN 100
#define GROUP_LEN 200
#define MAX_TODOS 200
#define TODO_FILE "todos.dat"
#define FILE_NAME "alarms.dat"

typedef struct {
    int id;
    char title[TITLE_LEN];
    char participants[GROUP_LEN];
    int is_meeting;
    time_t when;
    int repeat_daily;
    int snooze_minutes;
    int active;
    int status; /* 0=pending, 1=completed, 2=missed */
    time_t last_fired;
} Alarm;

typedef struct {
    int id;
    char title[TITLE_LEN];
    int priority; /* 1=Critical, 2=High, 3=Medium, 4=Low */
    int completed;
    time_t due;
    int has_due;
    int due_alerted;
} Todo;

static Alarm alarms[MAX_ITEMS];
static int count = 0;
static int next_id = 1;
static Todo todos[MAX_TODOS];
static int todo_count = 0;
static int next_todo_id = 1;

static void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}

static void read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }
    trim_newline(buf);
}

static int read_int(const char *prompt, int min, int max) {
    char buf[64], *end;
    long v;
    for (;;) {
        read_line(prompt, buf, sizeof buf);
        errno = 0;
        v = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t') end++;
        if (!errno && end != buf && *end == '\0' && v >= min && v <= max)
            return (int)v;
        printf("Enter a number from %d to %d.\n", min, max);
    }
}

static void save_data(void) {
    FILE *f = fopen(FILE_NAME, "w");
    if (!f) {
        perror("Could not save alarms.dat");
        return;
    }
    for (int i = 0; i < count; i++) {
        Alarm *a = &alarms[i];
        fprintf(f, "%d|%lld|%d|%d|%d|%d|%lld|%d|%s|%s\n",
                a->id, (long long)a->when, a->repeat_daily,
                a->snooze_minutes, a->active, a->status,
                (long long)a->last_fired, a->is_meeting,
                a->title, a->participants);
    }
    fclose(f);
}

static void load_data(void) {
    FILE *f = fopen(FILE_NAME, "r");
    if (!f) return;
    char line[512];
    while (count < MAX_ITEMS && fgets(line, sizeof line, f)) {
        Alarm a = {0};
        long long when, fired;
        char title[TITLE_LEN];
        char participants[GROUP_LEN] = "";
        int is_meeting = 0;
        int fields = sscanf(line, "%d|%lld|%d|%d|%d|%d|%lld|%d|%99[^|]|%199[^\n]",
                            &a.id, &when, &a.repeat_daily, &a.snooze_minutes,
                            &a.active, &a.status, &fired, &is_meeting,
                            title, participants);
        if (fields == 10) {
            a.when = (time_t)when;
            a.last_fired = (time_t)fired;
            a.is_meeting = is_meeting;
            snprintf(a.title, sizeof a.title, "%s", title);
            snprintf(a.participants, sizeof a.participants, "%s", participants);
            alarms[count++] = a;
            if (a.id >= next_id) next_id = a.id + 1;
        } else {
            /* Read records saved by the earlier single-person version. */
            fields = sscanf(line, "%d|%lld|%d|%d|%d|%d|%lld|%99[^\n]",
                            &a.id, &when, &a.repeat_daily, &a.snooze_minutes,
                            &a.active, &a.status, &fired, title);
            if (fields == 8) {
                a.when = (time_t)when;
                a.last_fired = (time_t)fired;
                snprintf(a.title, sizeof a.title, "%s", title);
                alarms[count++] = a;
                if (a.id >= next_id) next_id = a.id + 1;
            }
        }
    }
    fclose(f);
}

static void format_time(time_t t, char *buf, size_t n) {
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(buf, n, "%Y-%m-%d %I:%M:%S %p", &tmv);
}

static int find_alarm(int id) {
    for (int i = 0; i < count; i++)
        if (alarms[i].id == id) return i;
    return -1;
}

static void add_alarm(void) {
    if (count >= MAX_ITEMS) {
        puts("Alarm limit reached.");
        return;
    }
    Alarm a = {0};
    char title[TITLE_LEN], date[32], clockstr[32];
    read_line("Activity name (e.g., Study DSA): ", title, sizeof title);
    if (!title[0] || strchr(title, '|')) {
        puts("Activity name cannot be empty or contain '|'.");
        return;
    }
    read_line("Date (YYYY-MM-DD): ", date, sizeof date);
    read_line("Time (HH:MM, 24-hour): ", clockstr, sizeof clockstr);
    int y, mo, d, h, mi;
    char extra;
    if (sscanf(date, "%d-%d-%d%c", &y, &mo, &d, &extra) != 3 ||
        sscanf(clockstr, "%d:%d%c", &h, &mi, &extra) != 2 ||
        mo < 1 || mo > 12 || d < 1 || d > 31 ||
        h < 0 || h > 23 || mi < 0 || mi > 59) {
        puts("Invalid date or time format.");
        return;
    }
    struct tm tmv = {0};
    tmv.tm_year = y - 1900; tmv.tm_mon = mo - 1; tmv.tm_mday = d;
    tmv.tm_hour = h; tmv.tm_min = mi; tmv.tm_sec = 0; tmv.tm_isdst = -1;
    time_t t = mktime(&tmv);
    struct tm check;
    localtime_r(&t, &check);
    if (t == (time_t)-1 || check.tm_year != y-1900 || check.tm_mon != mo-1 ||
        check.tm_mday != d || t <= time(NULL)) {
        puts("That date/time is invalid or is not in the future.");
        return;
    }
    a.id = next_id++;
    snprintf(a.title, sizeof a.title, "%s", title);
    a.when = t;
    a.active = 1;
    a.status = 0;
    a.is_meeting = read_int("Is this a group meeting reminder? (1=yes, 0=no): ", 0, 1);
    if (a.is_meeting) {
        read_line("Participants (comma-separated names): ",
                  a.participants, sizeof a.participants);
        if (strchr(a.participants, '|')) {
            puts("Participant names cannot contain '|'.");
            return;
        }
    }
    a.repeat_daily = read_int("Repeat daily? (1=yes, 0=no): ", 0, 1);
    a.snooze_minutes = 5;
    alarms[count++] = a;
    save_data();
    puts("Alarm created and saved.");
}

static void list_alarms(void) {
    if (!count) { puts("No activities scheduled."); return; }
    puts("\nID   DATE & TIME             TYPE       STATUS      ACTIVITY");
    puts("--------------------------------------------------------------------------");
    for (int i = 0; i < count; i++) {
        Alarm *a = &alarms[i];
        char t[64]; format_time(a->when, t, sizeof t);
        const char *status = !a->active ? "disabled" :
            a->status == 1 ? "completed" : a->status == 2 ? "missed" : "pending";
        printf("%-4d %-22s %-10s %-11s %s%s\n", a->id, t,
               a->repeat_daily ? "daily" : "once", status, a->title,
               a->is_meeting ? " [GROUP MEETING]" : "");
        if (a->is_meeting && a->participants[0])
            printf("     Participants: %s\n", a->participants);
    }
}

static void mark_status(void) {
    list_alarms();
    if (!count) return;
    int id = read_int("Enter activity ID: ", 1, 1000000);
    int i = find_alarm(id);
    if (i < 0) { puts("ID not found."); return; }
    puts("1. Mark completed\n2. Mark missed\n3. Set pending");
    int s = read_int("Choose status: ", 1, 3);
    alarms[i].status = s == 1 ? 1 : s == 2 ? 2 : 0;
    save_data();
    puts("Activity status updated.");
}

static void reschedule_meeting(void) {
    list_alarms();
    if (!count) return;
    int id = read_int("Enter meeting/activity ID to reschedule: ", 1, 1000000);
    int i = find_alarm(id);
    if (i < 0) { puts("ID not found."); return; }
    if (!alarms[i].is_meeting) {
        puts("This activity is not marked as a group meeting.");
        return;
    }
    char date[32], clockstr[32];
    read_line("New date (YYYY-MM-DD): ", date, sizeof date);
    read_line("New time (HH:MM, 24-hour): ", clockstr, sizeof clockstr);
    int y, mo, d, h, mi;
    char extra;
    if (sscanf(date, "%d-%d-%d%c", &y, &mo, &d, &extra) != 3 ||
        sscanf(clockstr, "%d:%d%c", &h, &mi, &extra) != 2 ||
        mo < 1 || mo > 12 || d < 1 || d > 31 ||
        h < 0 || h > 23 || mi < 0 || mi > 59) {
        puts("Invalid date or time format.");
        return;
    }
    struct tm tmv = {0};
    tmv.tm_year = y - 1900; tmv.tm_mon = mo - 1; tmv.tm_mday = d;
    tmv.tm_hour = h; tmv.tm_min = mi; tmv.tm_sec = 0; tmv.tm_isdst = -1;
    time_t t = mktime(&tmv);
    struct tm check;
    if (t == (time_t)-1) {
        puts("Invalid date/time.");
        return;
    }
    localtime_r(&t, &check);
    if (check.tm_year != y-1900 || check.tm_mon != mo-1 ||
        check.tm_mday != d || t <= time(NULL)) {
        puts("The new date/time is invalid or is not in the future.");
        return;
    }
    alarms[i].when = t;
    alarms[i].active = 1;
    alarms[i].status = 0;
    alarms[i].last_fired = 0;
    save_data();
    char formatted[64]; format_time(t, formatted, sizeof formatted);
    printf("Meeting rescheduled to %s. The updated time is saved locally.\n", formatted);
    puts("Share the new time with participants; this terminal app does not send invitations.");
}

static void delete_alarm(void) {
    list_alarms();
    if (!count) return;
    int id = read_int("Enter ID to delete: ", 1, 1000000);
    int i = find_alarm(id);
    if (i < 0) { puts("ID not found."); return; }
    for (int j = i; j < count - 1; j++) alarms[j] = alarms[j+1];
    count--;
    save_data();
    puts("Activity deleted.");
}

static void show_upcoming(void) {
    int best = -1;
    time_t now = time(NULL);
    for (int i = 0; i < count; i++)
        if (alarms[i].active && alarms[i].status == 0 &&
            alarms[i].when >= now && (best < 0 || alarms[i].when < alarms[best].when))
            best = i;
    if (best < 0) { puts("No upcoming pending activities."); return; }
    char t[64]; format_time(alarms[best].when, t, sizeof t);
    printf("Next activity: %s at %s\n", alarms[best].title, t);
}

static void daily_overview(void) {
    time_t now = time(NULL);
    struct tm today; localtime_r(&now, &today);
    int planned=0, done=0, missed=0, pending=0;
    puts("\nTODAY'S OVERVIEW");
    for (int i = 0; i < count; i++) {
        struct tm at; localtime_r(&alarms[i].when, &at);
        if (at.tm_year == today.tm_year && at.tm_yday == today.tm_yday) {
            char t[32]; strftime(t, sizeof t, "%I:%M %p", &at);
            printf("%s  %-25s  %s\n", t, alarms[i].title,
                alarms[i].status == 1 ? "Completed" :
                alarms[i].status == 2 ? "Missed" : "Pending");
            planned++;
            if (alarms[i].status == 1) done++;
            else if (alarms[i].status == 2) missed++;
            else pending++;
        }
    }
    if (!planned) puts("No activities scheduled for today.");
    printf("Total: %d | Completed: %d | Missed: %d | Pending: %d\n",
           planned, done, missed, pending);
}

/* Generate a short two-tone WAV file so no external sound asset is needed. */
static void write_u16(FILE *f, uint16_t v) {
    fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f);
}
static void write_u32(FILE *f, uint32_t v) {
    write_u16(f, (uint16_t)(v & 0xffff));
    write_u16(f, (uint16_t)(v >> 16));
}
static int create_alarm_tone(const char *path, int priority) {
    const unsigned rate = 22050, seconds = 1, samples = rate * seconds;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite("RIFF", 1, 4, f); write_u32(f, 36 + samples * 2);
    fwrite("WAVEfmt ", 1, 8, f); write_u32(f, 16);
    write_u16(f, 1); write_u16(f, 1);
    write_u32(f, rate); write_u32(f, rate * 2);
    write_u16(f, 2); write_u16(f, 16);
    fwrite("data", 1, 4, f); write_u32(f, samples * 2);
    for (unsigned n = 0; n < samples; n++) {
        double first = priority <= 1 ? 1200.0 : priority == 2 ? 1000.0 : 880.0;
        double second = priority <= 1 ? 900.0 : priority == 2 ? 750.0 : 660.0;
        double freq = ((n / (rate / 4)) % 2 == 0) ? first : second;
        double phase = 6.283185307179586 * freq * n / rate;
        /* Brief fade at each edge prevents clicks. */
        unsigned edge = n % (rate / 4);
        double envelope = 1.0;
        if (edge < 300) envelope = (double)edge / 300.0;
        if (edge > rate / 4 - 300) envelope = (double)(rate / 4 - edge) / 300.0;
        int16_t sample = (int16_t)(12000.0 * envelope * sin(phase));
        write_u16(f, (uint16_t)sample);
    }
    fclose(f);
    return 1;
}
static pid_t alarm_sound_pid = -1;

/* Play the generated alarm tone repeatedly until stop_alarm_sound() is called. */
static void play_alarm_sound(int priority) {
    const char *wav = "/tmp/smart_alarm_clock_alarm.wav";
    if (!create_alarm_tone(wav, priority)) {
        fputc('\a', stdout); fflush(stdout); return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0); /* Put the sound loop and its players in a stoppable group. */
        for (;;) {
            pid_t player = fork();
            if (player == 0) {
#if defined(__APPLE__)
                execlp("afplay", "afplay", wav, (char *)NULL);
#endif
                execlp("paplay", "paplay", wav, (char *)NULL);
                execlp("aplay", "aplay", "-q", wav, (char *)NULL);
                _exit(127);
            }
            if (player < 0) _exit(1);
            int status;
            while (waitpid(player, &status, 0) < 0 && errno == EINTR) {}
            /* If no audio player exists, avoid an endless busy loop. */
            if (WIFEXITED(status) && WEXITSTATUS(status) == 127) _exit(127);
        }
    }
    if (pid < 0) {
        fputc('\a', stdout); fflush(stdout);
        return;
    }
    setpgid(pid, pid);
    alarm_sound_pid = pid;
}

static void stop_alarm_sound(void) {
    if (alarm_sound_pid > 0) {
        kill(-alarm_sound_pid, SIGTERM);
        while (waitpid(alarm_sound_pid, NULL, 0) < 0 && errno == EINTR) {}
        alarm_sound_pid = -1;
    }
}

static void ring_alarm(int i) {
    Alarm *a = &alarms[i];
    printf("\n\n\a========================================\n");
    printf("  SMART ALARM: %s\n", a->title);
    printf("========================================\n");
    fflush(stdout);
    play_alarm_sound(3);
    puts("1. Snooze for 5 minutes");
    puts("2. Mark completed");
    puts("3. Mark missed");
    printf("Choose an action: ");
    fflush(stdout);
    char buf[32];
    if (!fgets(buf, sizeof buf, stdin)) {
        stop_alarm_sound();
        /* If input closes, leave the alarm pending for the next run. */
        return;
    }
    stop_alarm_sound();
    int action = atoi(buf);
    if (action == 1) {
        a->when = time(NULL) + 5 * 60;
        a->active = 1;
        a->status = 0;
        printf("Snoozed until ");
        char t[64]; format_time(a->when, t, sizeof t); puts(t);
    } else if (action == 2) {
        a->status = 1;
        if (a->repeat_daily) {
            struct tm next; localtime_r(&a->when, &next);
            next.tm_mday += 1; next.tm_sec = 0; next.tm_isdst = -1;
            a->when = mktime(&next);
            a->status = 0;
        } else {
            a->active = 0;
        }
        puts("Activity marked completed.");
    } else if (action == 3) {
        a->status = 2;
        if (a->repeat_daily) {
            struct tm next; localtime_r(&a->when, &next);
            next.tm_mday += 1; next.tm_sec = 0; next.tm_isdst = -1;
            a->when = mktime(&next);
            a->status = 0;
        } else {
            a->active = 0;
        }
        puts("Activity marked missed.");
    } else {
        puts("Invalid choice; alarm remains pending.");
        return;
    }
    a->last_fired = time(NULL);
    save_data();
}

static const char *priority_name(int p);
static void save_todos(void);

static void check_alarms(void) {
    time_t now = time(NULL);
    for (int i = 0; i < count; i++) {
        Alarm *a = &alarms[i];
        if (a->active && a->status == 0 && a->when <= now &&
            (a->last_fired == 0 || now - a->last_fired > 30)) {
            ring_alarm(i);
        }
    }
    for (int i = 0; i < todo_count; i++) {
        Todo *t = &todos[i];
        if (t->has_due && !t->completed && !t->due_alerted && t->due <= now) {
            printf("\n\n========== TO-DO DEADLINE ==========\n");
            printf("Task due: %s (Priority: %s)\n", t->title, priority_name(t->priority));
            play_alarm_sound(3);
            puts("1. Snooze for 5 minutes");
            puts("2. Stop reminder sound");
            printf("Choose an action: ");
            fflush(stdout);
            char response[32];
            if (!fgets(response, sizeof response, stdin)) {
                stop_alarm_sound();
                return;
            }
            stop_alarm_sound();
            int action = atoi(response);
            if (action == 1) {
                t->due = time(NULL) + 5 * 60;
                t->due_alerted = 0;
                puts("Task reminder snoozed for 5 minutes.");
            } else {
                t->due_alerted = 1;
                puts("Reminder sound stopped. Task remains pending.");
            }
            save_todos();
        }
    }
}

static const char *priority_name(int p) {
    switch (p) {
        case 1: return "CRITICAL";
        case 2: return "HIGH";
        case 3: return "MEDIUM";
        default: return "LOW";
    }
}

static void save_todos(void) {
    FILE *f = fopen(TODO_FILE, "w");
    if (!f) { perror("Could not save todos.dat"); return; }
    for (int i = 0; i < todo_count; i++) {
        Todo *t = &todos[i];
        fprintf(f, "%d|%d|%d|%d|%lld|%d|%s\n", t->id, t->priority,
                t->completed, t->has_due, (long long)t->due,
                t->due_alerted, t->title);
    }
    fclose(f);
}

static void load_todos(void) {
    FILE *f = fopen(TODO_FILE, "r");
    if (!f) return;
    char line[512], title[TITLE_LEN];
    while (todo_count < MAX_TODOS && fgets(line, sizeof line, f)) {
        Todo t = {0};
        long long due;
        int fields = sscanf(line, "%d|%d|%d|%d|%lld|%d|%99[^\n]",
                            &t.id, &t.priority, &t.completed, &t.has_due,
                            &due, &t.due_alerted, title);
        if (fields != 7) {
            t.due_alerted = 0;
            fields = sscanf(line, "%d|%d|%d|%d|%lld|%99[^\n]",
                            &t.id, &t.priority, &t.completed, &t.has_due,
                            &due, title);
        }
        if (fields == 7 || fields == 6) {
            t.due = (time_t)due;
            snprintf(t.title, sizeof t.title, "%s", title);
            todos[todo_count++] = t;
            if (t.id >= next_todo_id) next_todo_id = t.id + 1;
        }
    }
    fclose(f);
}

static void add_todo(void) {
    if (todo_count >= MAX_TODOS) { puts("To-do list is full."); return; }
    Todo t = {0};
    read_line("Task name: ", t.title, sizeof t.title);
    if (!t.title[0] || strchr(t.title, '|')) {
        puts("Task name cannot be empty or contain '|'."); return;
    }
    puts("Priority: 1=Critical, 2=High, 3=Medium, 4=Low");
    t.priority = read_int("Choose priority: ", 1, 4);
    t.has_due = read_int("Add a due date/time? (1=yes, 0=no): ", 0, 1);
    if (t.has_due) {
        char date[32], clockstr[32], extra;
        int y, mo, d, h, mi;
        read_line("Due date (YYYY-MM-DD): ", date, sizeof date);
        read_line("Due time (HH:MM, 24-hour): ", clockstr, sizeof clockstr);
        if (sscanf(date, "%d-%d-%d%c", &y, &mo, &d, &extra) != 3 ||
            sscanf(clockstr, "%d:%d%c", &h, &mi, &extra) != 2 ||
            mo < 1 || mo > 12 || d < 1 || d > 31 ||
            h < 0 || h > 23 || mi < 0 || mi > 59) {
            puts("Invalid date/time; task was not added."); return;
        }
        struct tm tmv = {0};
        tmv.tm_year=y-1900; tmv.tm_mon=mo-1; tmv.tm_mday=d;
        tmv.tm_hour=h; tmv.tm_min=mi; tmv.tm_isdst=-1;
        t.due=mktime(&tmv);
        struct tm check;
        if (t.due == (time_t)-1) { puts("Invalid date/time."); return; }
        localtime_r(&t.due, &check);
        if (check.tm_year != y-1900 || check.tm_mon != mo-1 ||
            check.tm_mday != d) { puts("Invalid calendar date."); return; }
    }
    t.id = next_todo_id++;
    todos[todo_count++] = t;
    save_todos();
    puts("Task added and saved.");
}

static void list_todos(void) {
    if (!todo_count) { puts("Your to-do list is empty."); return; }
    int order[MAX_TODOS];
    for (int i=0; i<todo_count; i++) order[i]=i;
    /* Stable insertion sort: unfinished first, then priority, then due date. */
    for (int i=1; i<todo_count; i++) {
        int key=order[i], j=i-1;
        while (j>=0) {
            Todo *a=&todos[order[j]], *b=&todos[key];
            int after = (a->completed < b->completed) ? 0 :
                (a->completed > b->completed) ? 1 :
                (a->priority < b->priority) ? 0 :
                (a->priority > b->priority) ? 1 :
                (a->has_due && b->has_due && a->due > b->due);
            if (!after) break;
            order[j+1]=order[j]; j--;
        }
        order[j+1]=key;
    }
    puts("\nTO-DO LIST (sorted by completion, priority, and due date)");
    puts("ID   PRIORITY   STATUS       DUE DATE & TIME       TASK");
    puts("--------------------------------------------------------------------------");
    for (int n=0; n<todo_count; n++) {
        Todo *t=&todos[order[n]];
        char due[64]="—";
        if (t->has_due) format_time(t->due,due,sizeof due);
        printf("%-4d %-10s %-12s %-21s %s\n",t->id,priority_name(t->priority),
               t->completed?"DONE":"PENDING",due,t->title);
        if (!t->completed && t->has_due && t->due < time(NULL))
            puts("     ** OVERDUE **");
    }
}

static void complete_todo(void) {
    list_todos();
    if (!todo_count) return;
    int id=read_int("Enter task ID to mark completed: ",1,1000000);
    for (int i=0;i<todo_count;i++) if (todos[i].id==id) {
        todos[i].completed=1; save_todos(); puts("Task marked completed."); return;
    }
    puts("Task ID not found.");
}

static void delete_todo(void) {
    list_todos();
    if (!todo_count) return;
    int id=read_int("Enter task ID to delete: ",1,1000000);
    for (int i=0;i<todo_count;i++) if (todos[i].id==id) {
        for (int j=i;j<todo_count-1;j++) todos[j]=todos[j+1];
        todo_count--; save_todos(); puts("Task deleted."); return;
    }
    puts("Task ID not found.");
}

static void show_menu(void) {
    puts("\n\n========== SMART ALARM CLOCK ==========");
    time_t now = time(NULL);
    char clock_text[64];
    format_time(now, clock_text, sizeof clock_text);
    printf("Current time: %s\n", clock_text);
    puts("1. Add alarm / activity");
    puts("2. View all alarms");
    puts("3. View upcoming activity");
    puts("4. Daily overview");
    puts("5. Update activity status");
    puts("6. Delete alarm / activity");
    puts("7. Reschedule a group meeting");
    puts("8. Save and exit");
    puts("9. Add to-do task");
    puts("10. View prioritized to-do list");
    puts("11. Mark to-do task completed");
    puts("12. Delete to-do task");
}

int main(void) {
    load_data();
    load_todos();
    puts("SMART ALARM CLOCK — Time Management & Productivity System");
    printf("Loaded %d saved activity(ies) and %d to-do task(s).\n",
           count, todo_count);
    puts("Keep this terminal open for live alarm monitoring.");
    show_menu();
    printf("\nEnter option (1-12): ");
    fflush(stdout);

    while (1) {
        check_alarms();

        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        struct timeval tv = {1, 0};
        int ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &tv);

        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        if (ready == 0) continue;

        char buf[64];
        if (!fgets(buf, sizeof buf, stdin)) break;
        int choice = atoi(buf);
        switch (choice) {
            case 1: add_alarm(); break;
            case 2: list_alarms(); break;
            case 3: show_upcoming(); break;
            case 4: daily_overview(); break;
            case 5: mark_status(); break;
            case 6: delete_alarm(); break;
            case 7: reschedule_meeting(); break;
            case 8: save_data(); save_todos(); puts("Saved. Goodbye!"); return 0;
            case 9: add_todo(); break;
            case 10: list_todos(); break;
            case 11: complete_todo(); break;
            case 12: delete_todo(); break;
            default: puts("Choose a number from 1 to 12.");
        }
        printf("\nEnter option (1-12): ");
        fflush(stdout);
    }
    save_data();
    save_todos();
    return 0;
}
