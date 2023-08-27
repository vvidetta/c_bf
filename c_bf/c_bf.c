#include "c_bf.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

void print_hello_world()
{
    puts("Hello World!");
}

static int const DEFAULT_BUFFER_SIZE = 30000;

BfBool BfMachine_Init(struct BfMachine* machine, struct BfIoDriver const* ioDriver)
{
  if (machine == NULL || ioDriver == NULL)
    return BfBool_False;
  machine->buffer_size = DEFAULT_BUFFER_SIZE;
  int* buffer = malloc(machine->buffer_size * sizeof(int));
  if (buffer == NULL)
    return BfBool_False;
  memset(buffer, 0, machine->buffer_size);
  machine->buffer = buffer;
  machine->data_pointer = 0;
  machine->instruction_pointer = 0;
  machine->program = NULL;
  machine->io_driver = ioDriver;
  return BfBool_True;
}

static int* CopyIntBuffer(int const* src, size_t count)
{

  size_t const buffer_length = count * sizeof(int);
  int* new_buffer = malloc(buffer_length);
  if (new_buffer == NULL)
    return NULL;
  memcpy(new_buffer, src, buffer_length);
  return new_buffer;
}

BfBool BfMachine_Copy(struct BfMachine* dest, struct BfMachine* src)
{
  if (dest == NULL || src == NULL)
    return BfBool_False;
  memcpy(dest, src, sizeof(struct BfMachine));

  dest->buffer = CopyIntBuffer(src->buffer, dest->buffer_size);
  if (dest->buffer == NULL)
    return BfBool_False;

  return BfBool_True;
}

BfBool BfMachine_Clean(struct BfMachine* machine)
{
  if (machine == NULL)
    return BfBool_False;

  machine->buffer_size = 0;
  free(machine->buffer);
  machine->buffer = NULL;
  machine->data_pointer = -1;
  machine->instruction_pointer = -1;
  machine->program = NULL;
  machine->io_driver = NULL;
  return BfBool_True;
}

BfBool BfMachine_LoadProgram(struct BfMachine* machine, char const* program)
{
  if (machine == NULL)
    return BfBool_False;
  if (program == NULL)
    return BfBool_False;
  machine->program = program;
  return BfBool_True;
}

BfBool BfMachine_ClearProgram(struct BfMachine* machine)
{
  if (machine == NULL)
    return BfBool_False;
  machine->program = NULL;
  return BfBool_True;
}

BfBool BfMachine_ExecuteProgram(struct BfMachine* machine)
{
  if (machine == NULL || machine->program == NULL)
    return BfBool_False;

  BfIoDriver_ReadValue const read_value = machine->io_driver != NULL ? machine->io_driver->read_value_fn : NULL;
  BfIoDriver_WriteValue const write_value = machine->io_driver != NULL ? machine->io_driver->write_value_fn : NULL;
  for (
    char instruction = machine->program[machine->instruction_pointer];
    instruction != '\0';
    ++machine->instruction_pointer, instruction = machine->program[machine->instruction_pointer]
    )
  {
    switch (instruction)
    {
    default:
      return BfBool_False;

    case '+':
      ++machine->buffer[machine->data_pointer];
      break;

    case '-':
      --machine->buffer[machine->data_pointer];
      break;

    case '>':
      if (machine->data_pointer == machine->buffer_size - 1)
        return BfBool_False;
      ++machine->data_pointer;
      break;

    case '<':
      if (machine->data_pointer == 0)
        return BfBool_False;
      --machine->data_pointer;
      break;

    case '.':
      if (read_value != NULL)
        machine->buffer[0] = read_value();
      break;

    case ',':
      if (write_value != NULL)
        write_value(machine->io_driver->context, 0);
      break;
    }
  }

  return BfBool_True;
}

