#ifndef DISTRICT_H
#define DISTRICT_H

typedef struct {
    int time;
    double consumption;
} SchedulePoint;

typedef struct {
    int id;
    SchedulePoint *Schedule;
    SchedulePoint *current_point;
} District;

#endif
