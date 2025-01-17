#include "./WASITypes.h"
#include "WAVM/IR/Types.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/HashMap.h"
#include "WAVM/Inline/I128.h"
#include "WAVM/Inline/IndexMap.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/WASI/WASI.h"

// Macros for tracing syscalls
#define TRACE_SYSCALL(syscallName, argFormat, ...)                                                 \
	const char* TRACE_SYSCALL_name = syscallName;                                                  \
	traceSyscallf(TRACE_SYSCALL_name, argFormat, ##__VA_ARGS__)

#define TRACE_SYSCALL_RETURN(returnCode, ...)                                                      \
	traceSyscallReturnf(TRACE_SYSCALL_name, returnCode, " " __VA_ARGS__)

#define UNIMPLEMENTED_SYSCALL(syscallName, argFormat, ...)                                         \
	TRACE_SYSCALL(syscallName, argFormat, ##__VA_ARGS__);                                          \
	Log::printf(Log::error, "Called unimplemented WASI syscall %s.\n", syscallName);               \
	return TRACE_SYSCALL_RETURN(__WASI_ENOSYS);

// Operations that apply to regular files.
#define REGULAR_FILE_RIGHTS                                                                        \
	(__WASI_RIGHT_FD_DATASYNC | __WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_SEEK                        \
	 | __WASI_RIGHT_FD_FDSTAT_SET_FLAGS | __WASI_RIGHT_FD_SYNC | __WASI_RIGHT_FD_TELL              \
	 | __WASI_RIGHT_FD_WRITE | __WASI_RIGHT_FD_ADVISE | __WASI_RIGHT_FD_ALLOCATE                   \
	 | __WASI_RIGHT_FD_FILESTAT_GET | __WASI_RIGHT_FD_FILESTAT_SET_SIZE                            \
	 | __WASI_RIGHT_FD_FILESTAT_SET_TIMES | __WASI_RIGHT_POLL_FD_READWRITE)

// Only allow directory operations on directories.
#define DIRECTORY_RIGHTS                                                                           \
	(__WASI_RIGHT_FD_FDSTAT_SET_FLAGS | __WASI_RIGHT_FD_SYNC | __WASI_RIGHT_FD_ADVISE              \
	 | __WASI_RIGHT_PATH_CREATE_DIRECTORY | __WASI_RIGHT_PATH_CREATE_FILE                          \
	 | __WASI_RIGHT_PATH_LINK_SOURCE | __WASI_RIGHT_PATH_LINK_TARGET | __WASI_RIGHT_PATH_OPEN      \
	 | __WASI_RIGHT_FD_READDIR | __WASI_RIGHT_PATH_READLINK | __WASI_RIGHT_PATH_RENAME_SOURCE      \
	 | __WASI_RIGHT_PATH_RENAME_TARGET | __WASI_RIGHT_PATH_FILESTAT_GET                            \
	 | __WASI_RIGHT_PATH_FILESTAT_SET_SIZE | __WASI_RIGHT_PATH_FILESTAT_SET_TIMES                  \
	 | __WASI_RIGHT_FD_FILESTAT_GET | __WASI_RIGHT_FD_FILESTAT_SET_TIMES                           \
	 | __WASI_RIGHT_PATH_SYMLINK | __WASI_RIGHT_PATH_UNLINK_FILE                                   \
	 | __WASI_RIGHT_PATH_REMOVE_DIRECTORY | __WASI_RIGHT_POLL_FD_READWRITE)
// Only allow directory or file operations to be derived from directories.
#define INHERITING_DIRECTORY_RIGHTS (DIRECTORY_RIGHTS | REGULAR_FILE_RIGHTS)

namespace WAVM { namespace VFS {
	enum class Result;
	struct DirEntStream;
	struct VFD;
}}

namespace WAVM { namespace WASI {
	struct FDE
	{
		VFS::VFD* vfd;
		__wasi_rights_t rights;
		__wasi_rights_t inheritingRights;

		std::string originalPath;

		bool isPreopened;
		__wasi_preopentype_t preopenedType;

		VFS::DirEntStream* dirEntStream{nullptr};

		FDE(VFS::VFD* inVFD,
			__wasi_rights_t inRights,
			__wasi_rights_t inInheritingRights,
			std::string&& inOriginalPath,
			bool inIsPreopened = false,
			__wasi_preopentype_t inPreopenedType = __WASI_PREOPENTYPE_DIR)
		: vfd(inVFD)
		, rights(inRights)
		, inheritingRights(inInheritingRights)
		, originalPath(std::move(inOriginalPath))
		, isPreopened(inIsPreopened)
		, preopenedType(inPreopenedType)
		{
		}

		VFS::Result close() const;
	};

	struct ProcessResolver : Runtime::Resolver
	{
		HashMap<std::string, Runtime::GCPointer<Runtime::ModuleInstance>> moduleNameToInstanceMap;

		bool resolve(const std::string& moduleName,
					 const std::string& exportName,
					 IR::ExternType type,
					 Runtime::Object*& outObject) override;
	};

	struct Process
	{
		Runtime::GCPointer<Runtime::Compartment> compartment;
		Runtime::GCPointer<Runtime::Context> context;
		Runtime::GCPointer<Runtime::Memory> memory;
		Runtime::GCPointer<Runtime::ModuleInstance> moduleInstance;
		std::vector<std::string> args;
		std::vector<std::string> envs;

		IndexMap<__wasi_fd_t, WASI::FDE> fds{0, INT32_MAX};
		VFS::FileSystem* fileSystem = nullptr;

		ProcessResolver resolver;

		I128 processClockOrigin;

		~Process();
	};

	typedef U32 WASIAddress;
#define WASIADDRESS_MAX UINT32_MAX
#define WASIADDRESS_FORMAT "0x%08x"

	// __wasi_errno_t is actually 16-bits but since WebAssembly doesn't have an I16 type, we need to
	// return an I32 from the intrinsic functions.
	typedef uint32_t __wasi_errno_return_t;

	inline Process* getProcessFromContextRuntimeData(
		Runtime::ContextRuntimeData* contextRuntimeData)
	{
		return (Process*)Runtime::getUserData(
			Runtime::getCompartmentFromContextRuntimeData(contextRuntimeData));
	}

	WAVM_VALIDATE_AS_PRINTF(2, 3)
	void traceSyscallf(const char* syscallName, const char* argFormat, ...);

	WAVM_VALIDATE_AS_PRINTF(3, 4)
	__wasi_errno_t traceSyscallReturnf(const char* syscallName,
									   __wasi_errno_t result,
									   const char* format,
									   ...);

	WAVM_DECLARE_INTRINSIC_MODULE(wasi);
	WAVM_DECLARE_INTRINSIC_MODULE(wasiArgsEnvs);
	WAVM_DECLARE_INTRINSIC_MODULE(wasiClocks);
	WAVM_DECLARE_INTRINSIC_MODULE(wasiFile);
}}
