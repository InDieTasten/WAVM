#include "WAVM/VFS/SandboxFS.h"
#include <string>
#include "WAVM/Inline/I128.h"
#include "WAVM/VFS/VFS.h"

using namespace WAVM;
using namespace WAVM::VFS;

struct SandboxFS : FileSystem
{
	SandboxFS(FileSystem* inInnerFS, const std::string& inRootPath)
	: innerFS(inInnerFS), rootPath(inRootPath)
	{
		if(rootPath.back() != '/' && rootPath.back() != '\\') { rootPath += '/'; }
	}

	virtual Result open(const std::string& path,
						FileAccessMode accessMode,
						FileCreateMode createMode,
						VFD*& outFD,
						const VFDFlags& flags) override
	{
		return innerFS->open(getInnerPath(path), accessMode, createMode, outFD, flags);
	}

	virtual Result getFileInfo(const std::string& path, FileInfo& outInfo) override
	{
		return innerFS->getFileInfo(getInnerPath(path), outInfo);
	}
	virtual Result setFileTimes(const std::string& path,
								bool setLastAccessTime,
								I128 lastAccessTime,
								bool setLastWriteTime,
								I128 lastWriteTime) override
	{
		return innerFS->setFileTimes(
			getInnerPath(path), setLastAccessTime, lastAccessTime, setLastWriteTime, lastWriteTime);
	}

	virtual Result openDir(const std::string& path, DirEntStream*& outStream) override
	{
		return innerFS->openDir(getInnerPath(path), outStream);
	}

	virtual Result unlinkFile(const std::string& path) override
	{
		return innerFS->unlinkFile(getInnerPath(path));
	}

	virtual Result removeDir(const std::string& path) override
	{
		return innerFS->removeDir(getInnerPath(path));
	}

	virtual Result createDir(const std::string& path) override
	{
		return innerFS->createDir(getInnerPath(path));
	}

private:
	VFS::FileSystem* innerFS;
	std::string rootPath;

	std::string getInnerPath(const std::string& absolutePathName)
	{
		return rootPath + absolutePathName;
	}
};

FileSystem* VFS::makeSandboxFS(FileSystem* innerFS, const std::string& innerRootPath)
{
	return new SandboxFS(innerFS, innerRootPath);
}
