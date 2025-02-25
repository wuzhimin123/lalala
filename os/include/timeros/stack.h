#ifndef TOS_STACK_H__
#define TOS_STACK_H__

#include "os.h"

#define MAX_SIZE 10000

typedef struct{
    u64 data[MAX_SIZE];
    int top;
}Stack;

bool isEmpty(Stack *stack);
bool isFull(Stack *stack);
void push(Stack *stack, u64 value);
u64 pop(Stack *stack);
u64 top(Stack *stack);

#endif