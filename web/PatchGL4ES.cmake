find_program(GIT_EXECUTABLE git REQUIRED)
set(PATCH "${CMAKE_CURRENT_LIST_DIR}/gl4es-default-texture.patch")

# FetchContent may revisit its patch step on reconfiguration. Accept only the
# exact patch or its already-applied form, never a partially matching edit.
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH}"
	WORKING_DIRECTORY "${SOURCE_DIR}" RESULT_VARIABLE ALREADY_APPLIED
	OUTPUT_QUIET ERROR_QUIET)
if(NOT ALREADY_APPLIED EQUAL 0)
	execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${PATCH}"
		WORKING_DIRECTORY "${SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
	execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${PATCH}"
		WORKING_DIRECTORY "${SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
endif()
