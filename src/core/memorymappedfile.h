#ifndef MEMORYMAPPEDFILE_H
#define MEMORYMAPPEDFILE_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class MemoryMappedFile
{
public:
	explicit MemoryMappedFile(const char *path)
	{
#ifdef _WIN32
		WIN32_FILE_ATTRIBUTE_DATA attr;
		if (!GetFileAttributesEx(path, GetFileExInfoStandard, &attr))
			throw std::runtime_error("failed to get file attributes");
		size = attr.nFileSizeLow;

		if (attr.nFileSizeHigh != 0)
			throw std::runtime_error("too large file");

		hfile = CreateFile(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (INVALID_HANDLE_VALUE == hfile)
			throw std::runtime_error("failed to open file for reading");

		hmap = CreateFileMapping(hfile, 0, PAGE_READONLY, 0, 0, nullptr);
		if (!hmap)
			throw std::runtime_error("failed to create file mapping");

		data = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
		if (!data)
			throw std::runtime_error("failed to map view of file");
#else
		fd = open(path, O_RDONLY);
		if (fd < 0)
			throw std::runtime_error("failed to open file for reading");

		struct stat st;
		if (fstat(fd, &st) < 0)
			throw std::runtime_error("failed to stat");

		size = st.st_size;
		data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (data == MAP_FAILED)
			throw std::runtime_error("failed to mmap file");
#endif
	}

	~MemoryMappedFile()
	{
#ifdef _WIN32
		UnmapViewOfFile(data);
		CloseHandle(hmap);
		CloseHandle(hfile);
#else
		close(fd);
#endif
	}

	const void *getData() const { return data; }
	size_t getSize() const { return size; }

private:
#ifdef _WIN32
	HANDLE hfile;
	HANDLE hmap;
#else
	int fd;
#endif
	void *data;
	size_t size;
};

#endif // MEMORYMAPPEDFILE_H
