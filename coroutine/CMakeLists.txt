set(
	source_files
	src/H2OFastTests_coro.cpp
)

source_group(
	"Sources"
	FILES
	$(source_files)
)

add_executable(Coroutine ${source_files})
set_target_properties(Coroutine PROPERTIES LINKER_LANGUAGE CXX)