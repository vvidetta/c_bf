#include "c_bf.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#define DISABLE_SEMANTICS(className, semantics) \
  className(className semantics) = delete;\
  className& operator=(className semantics) = delete
#define COPY_SEMANTICS const&
#define DISABLE_COPY(className) DISABLE_SEMANTICS(className, COPY_SEMANTICS)
#define MOVE_SEMANTICS &&
#define DISABLE_MOVE(className) DISABLE_SEMANTICS(className, MOVE_SEMANTICS)

class BfMachineWrapper final
{
public:
  BfMachineWrapper()
    : BfMachineWrapper(BfIoDriver{})
  {
    // Need to call init outside of assert to ensure that it is executed in Release builds
    auto result = BfMachine_Init(&m_machine, &m_ioDriver);
    assert(result == BfBool_True);
  }

  BfMachineWrapper(BfIoDriver const& ioDriver)
    : m_ioDriver{ ioDriver },
    m_machine{}
  {
    // Need to call init outside of assert to ensure that it is executed in Release builds
    auto result = BfMachine_Init(&m_machine, &m_ioDriver);
    assert(result == BfBool_True);
  }

  ~BfMachineWrapper()
  {
    // Need to call init outside of assert to ensure that it is executed in Release builds
    auto result = BfMachine_Clean(&m_machine);
    assert(result == BfBool_True);
  }

  BfMachineWrapper(BfMachineWrapper& wrapper)
    : m_ioDriver{ wrapper.m_ioDriver }
  {
    // Need to call init outside of assert to ensure that it is executed in Release builds
    auto result = BfMachine_Copy(&m_machine, &wrapper.m_machine);
    assert(result == BfBool_True);
  }

  DISABLE_COPY(BfMachineWrapper);
  DISABLE_MOVE(BfMachineWrapper);

  [[nodiscard]] BfMachine& get()
  {
    return m_machine;
  }

private:
  struct BfIoDriver const m_ioDriver;
  struct BfMachine m_machine;
};

std::ostream& operator<<(std::ostream& os, BfIoDriver const& value)
{
  return os << "{\n"
    "read_value_fn = " << value.read_value_fn << "\n"
    "}";
}

std::ostream& operator<<(std::ostream& os, BfMachine const& value)
{
  return os << "{" << std::endl
    << "buffer_size = " << value.buffer_size << std::endl
    << "buffer = " << value.buffer << std::endl
    << "data_pointer = " << value.data_pointer << std::endl
    << "instruction_pointer = " << value.instruction_pointer << std::endl
    << "program = " << std::hex << static_cast<void const*>(value.program) << std::dec << std::endl
    << "io_driver = " << value.io_driver << std::endl
    << "}";
}

bool operator==(BfMachine const& lhs, BfMachine const& rhs)
{
  if (lhs.buffer == rhs.buffer)
    return lhs.buffer_size == rhs.buffer_size
      && lhs.data_pointer == rhs.data_pointer
      && lhs.instruction_pointer == rhs.instruction_pointer
      && lhs.program == rhs.program;

  if (std::memcmp(lhs.buffer, rhs.buffer, lhs.buffer_size * sizeof(int)) != 0)
    return false;

  return lhs.buffer_size == rhs.buffer_size
    && lhs.data_pointer == rhs.data_pointer
    && lhs.instruction_pointer == rhs.instruction_pointer
    && lhs.program == rhs.program;
}

using Wrapper = BfMachineWrapper;

TEST(BfMachineTests, CheckBfMachineInitReturnsBfFalseWhenGivenNullMachine)
{
  auto ioDriver = BfIoDriver{};
  ASSERT_EQ(BfMachine_Init(NULL, &ioDriver), BfBool_False);
}

TEST(BfMachineTests, CheckInitReturnsFalseWhenGivenNullIoDriver)
{
  auto machine = BfMachine{};
  ASSERT_EQ(BfMachine_Init(&machine, NULL), BfBool_False);
}

TEST(BfMachineTests, CheckBfMachineCleanReturnsFalseWhenGivenNullMachine)
{
  ASSERT_EQ(BfMachine_Clean(NULL), BfBool_False);
}

TEST(BfMachineTests, GivenABfMachineCheckThatItIsInitializedAndCleanedCorrectly)
{
  auto constexpr ioDriver = BfIoDriver{};
  struct BfMachine machine;

  ASSERT_EQ(BfMachine_Init(&machine, &ioDriver), BfBool_True);
  auto constexpr expectedBufferSize = 30000;
  EXPECT_EQ(machine.buffer_size, expectedBufferSize);
  EXPECT_NE(machine.buffer, (int *)NULL);
  // TODO: check that malloc allocated the correct amount of memory (expectedBufferSize * sizeof(int))
  //ASSERT_EQ(sizeof machine.buffer, expectedBufferSize * sizeof(int));
  auto const expectedBuffer = std::vector<int>(expectedBufferSize);
  EXPECT_TRUE(memcmp(machine.buffer, expectedBuffer.data(), expectedBufferSize) == 0);
  EXPECT_EQ(machine.data_pointer, 0);
  EXPECT_EQ(machine.instruction_pointer, 0);
  EXPECT_EQ(machine.program, nullptr);
  EXPECT_EQ(machine.io_driver, &ioDriver);

  auto const NonTrivialProgram = std::string("+");
  machine.program = NonTrivialProgram.c_str();

  ASSERT_EQ(BfMachine_Clean(&machine), BfBool_True);
  ASSERT_EQ(machine.buffer_size, 0);
  // TODO: check that the buffer is freed.
  ASSERT_EQ(machine.buffer, nullptr);
  ASSERT_EQ(machine.data_pointer, -1);
  ASSERT_EQ(machine.instruction_pointer, -1);
  ASSERT_EQ(machine.program, nullptr);
  ASSERT_EQ(machine.io_driver, nullptr);
}

TEST(BfMachineTests, CheckLoadProgramReturnsFalseWhenGivenANullMachine)
{
  ASSERT_EQ(BfMachine_LoadProgram(nullptr, ""), BfBool_False);
}

TEST(BfMachineTests, CheckLoadingANullStringReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, NULL), BfBool_False);
}

TEST(BfMachineTests, CheckLoadingEmptyProgramSetsTheProgram)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  auto expectedMachine = machine;
  expectedMachine.program = "";

  ASSERT_EQ(BfMachine_LoadProgram(&machine, expectedMachine.program), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckClearProgramReturnsFalseWhenGivenANullMachine)
{
  ASSERT_EQ(BfMachine_ClearProgram(nullptr), BfBool_False);
}

TEST(BfMachineTests, GivenAProgramHasBeenLoadedCheckThatClearProgramSetsTheProgramToNull)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ""), BfBool_True);

  ASSERT_EQ(BfMachine_ClearProgram(&machine), BfBool_True);

  ASSERT_EQ(machine.program, nullptr);
}

TEST(BfMachineTests, CheckExecuteProgramReturnsFalseWhenGivenANullMachine)
{
  ASSERT_EQ(BfMachine_ExecuteProgram(nullptr), BfBool_False);
}

TEST(BfMachineTests, GivenNoProgramHasLoadedCheckThatExecuteProgramReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_False);
}

TEST(BfMachineTests, CheckExecutingEmptyProgramDoesNothingToTheMachine)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ""), BfBool_True);
  auto const expectedMachine = machine;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckCopyingFromNullMachineReturnsFalse)
{
  auto copy = BfMachine{};
  ASSERT_EQ(BfMachine_Copy(&copy, nullptr), BfBool_False);
}

TEST(BfMachineTests, CheckCopyingToNullFromInitializedMachineReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& src = wrapper.get();
  ASSERT_EQ(BfMachine_Copy(nullptr, &src), BfBool_False);
}

TEST(BfMachineTests, CheckCopyingFromInitializedMachineToUninitializedMachineCopiesValuesAndCreatesDistinctBuffer)
{
  auto wrapper = BfMachineWrapper{};
  auto& src = wrapper.get();
  auto constexpr nonDefaultValue = 42;
  src.buffer[0] = nonDefaultValue;
  src.data_pointer = nonDefaultValue;
  src.instruction_pointer = nonDefaultValue;

  auto dest = BfMachine{};
  ASSERT_EQ(BfMachine_Copy(&dest, &src), BfBool_True);

  EXPECT_EQ(dest.buffer_size, src.buffer_size);
  // TODO: check that new buffer was allocated.
  EXPECT_NE(dest.buffer, src.buffer);
  EXPECT_TRUE(std::memcmp(dest.buffer, src.buffer, dest.buffer_size) == 0);
  EXPECT_EQ(dest.data_pointer, src.data_pointer);
  EXPECT_EQ(dest.instruction_pointer, src.instruction_pointer);
  EXPECT_EQ(dest.program, src.program);
  ASSERT_EQ(BfMachine_Clean(&dest), BfBool_True);
}

TEST(BfMachineTests, CheckExecutingPlusIncrementsInstructionPointerAndTheValuePointedToByTheDataPointer)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "+"), BfBool_True);
  auto expectedMachine = machine;
  expectedMachine.instruction_pointer = 1;
  expectedMachine.buffer[expectedMachine.data_pointer] = 1;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingPlusPlusIncrementsInstructionPointerAndTheValuePointedToByTheDataPointerTwice)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "++"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.instruction_pointer = 2;
  expectedMachine.buffer[expectedMachine.data_pointer] = 2;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingMinusIncrementsInstructionPointerAndDecrementsTheValuePointedToByTheDataPointer)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "-"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.instruction_pointer = 1;
  expectedMachine.buffer[expectedMachine.data_pointer] = -1;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingProgramWithUnrecognizedSymbolReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "#"), BfBool_True);
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_False);
}

TEST(BfMachineTests, CheckExecutingLeftReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "<"), BfBool_True);
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_False);
}

TEST(BfMachineTests, CheckExecutingRightIncrementsTheInstructionPointerAndDataPointer)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ">"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 1;
  expectedMachine.instruction_pointer = 1;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightRightIncrementsTheInstructionPointerAndDataPointerTwice)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ">>"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 2;
  expectedMachine.instruction_pointer = 2;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightLeftSetsInstructionPointerTo2AndDataPointerTo0)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "><"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 0;
  expectedMachine.instruction_pointer = 2;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightRightLeftSetsInstructionPointerTo3AndDataPointerTo1)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ">><"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 1;
  expectedMachine.instruction_pointer = 3;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightPlusSetsValuePointedToByDataPointerTo1)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ">+"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 1;
  expectedMachine.buffer[expectedMachine.data_pointer] = 1;
  expectedMachine.instruction_pointer = 2;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightMinusSetsValuePointedToByDataPointerToMinus1)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ">-"), BfBool_True);
  auto expectedWrapper = BfMachineWrapper{ wrapper };
  auto& expectedMachine = expectedWrapper.get();
  expectedMachine.data_pointer = 1;
  expectedMachine.buffer[expectedMachine.data_pointer] = -1;
  expectedMachine.instruction_pointer = 2;

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine, expectedMachine);
}

TEST(BfMachineTests, CheckExecutingRightMoreTimesThanTheBufferSizeReturnsFalse)
{
  auto wrapper = BfMachineWrapper{};
  auto& machine = wrapper.get();
  auto const tooManyRights = std::string(machine.buffer_size + 1, '>');
  ASSERT_EQ(BfMachine_LoadProgram(&machine, tooManyRights.c_str()), BfBool_True);

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_False);
}

TEST(BfMachineTests, GivenTheIoDriversReadValueIsNullCheckThatExecutingDotReturnsTrue)
{
  auto ioDriver = BfIoDriver{};
  ioDriver.read_value_fn = nullptr;
  auto wrapper = BfMachineWrapper{ ioDriver };
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "."), BfBool_True);
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);
}

auto constexpr expectedReadValue = 42;

TEST(BfMachineTests, GivenTheIoDriverHasAReadValueFunctionCheckThatExecutingDotReadsIntoTheValuePointedToByTheDataPointer)
{
  auto ioDriver = BfIoDriver{};
  ioDriver.read_value_fn = []() -> int
  {
    return expectedReadValue;
  };
  auto wrapper = BfMachineWrapper{ ioDriver };
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, "."), BfBool_True);

  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);

  ASSERT_EQ(machine.buffer[machine.data_pointer], expectedReadValue);
}

TEST(BfMachineTests, GivenTheIoDriversWriteValueIsNullCheckThatExecutingCommaReturnsTrue)
{
  auto ioDriver = BfIoDriver{};
  ioDriver.write_value_fn = nullptr;
  auto wrapper = BfMachineWrapper{ ioDriver };
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ","), BfBool_True);
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);
}

class MockIoDriver
{
public:
  MOCK_METHOD(void, WriteValue, (int value), (const));

  static void DoWriteValue(void* context, int value)
  {
    auto& mockIoDriver = *static_cast<MockIoDriver*>(context);
    mockIoDriver.WriteValue(value);
  }
};

TEST(BfMachineTests, GivenTheIoDriverHasAWriteValueFunctionCheckThatExecutingCommaWritesTheValuePointedToByTheDataPointer)
{
  auto mockIoDriver = MockIoDriver{};
  EXPECT_CALL(mockIoDriver, WriteValue(0));

  auto ioDriver = BfIoDriver{};
  ioDriver.write_value_fn = &MockIoDriver::DoWriteValue;
  ioDriver.context = &mockIoDriver;
  auto wrapper = BfMachineWrapper{ ioDriver };
  auto& machine = wrapper.get();
  ASSERT_EQ(BfMachine_LoadProgram(&machine, ","), BfBool_True);
  ASSERT_EQ(BfMachine_ExecuteProgram(&machine), BfBool_True);
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
