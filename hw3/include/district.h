#ifndef DISTRICT_H
#define DISTRICT_H

#define MAX_SCHEDULE 16

typedef struct {
    int time;
    double consumption;
} SchedulePoint;

typedef struct {
    int id;
    int schedule_size;
    SchedulePoint schedule[MAX_SCHEDULE];
    int current_point;
} District;

#endif
