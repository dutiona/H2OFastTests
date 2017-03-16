set(
	source_files
	src/H2OFastTests_Tests.cpp
)

source_group(
	"Sources"
	FILES
	$(source_files)
)

add_executable(Tests ${source_files})
set_target_properties(Tests PROPERTIES LINKER_LANGUAGE CXX)