#ifndef C_BF_C_BF_H
#define C_BF_C_BF_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  typedef enum BfBool_
  {
    BfBool_False = 0,
    BfBool_True = -1
  } BfBool;

  extern void print_hello_world(void);

  typedef int (*BfIoDriver_ReadValue)(void);

  typedef void (*BfIoDriver_WriteValue)(void* context, int value);

  struct BfIoDriver
  {
    BfIoDriver_ReadValue read_value_fn;
    BfIoDriver_WriteValue write_value_fn;
    void* context;
  };

  struct BfMachine
  {
    int buffer_size;
    int* buffer;
    int data_pointer;
    int instruction_pointer;
    char const* program;
    struct BfIoDriver const* io_driver;
  };

  BfBool BfMachine_Init(struct BfMachine* machine, struct BfIoDriver const* ioDriver);

  BfBool BfMachine_Copy(struct BfMachine* dest, struct BfMachine* src);

  BfBool BfMachine_Clean(struct BfMachine* machine);

  BfBool BfMachine_ClearProgram(struct BfMachine* machine);

  BfBool BfMachine_LoadProgram(struct BfMachine* machine, char const* program);

  BfBool BfMachine_ExecuteProgram(struct BfMachine* machine);

#ifdef __cplusplus
}
#endif

#endif // C_BF_C_BF_H