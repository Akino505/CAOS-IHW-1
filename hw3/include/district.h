#ifndef DISTRICT_H
#define DISTRICT_H

typedef struct {
    int time;
    double consumption;
} SchedulePoint;

typedef struct {
    int id;
    int schedule_size;
    SchedulePoint *schedule;
    SchedulePoint *current_point;
} District;

#endif
