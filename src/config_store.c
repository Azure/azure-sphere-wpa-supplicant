#include "config_store.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

static char *AppendString(const char *front, const char *back)
{
    size_t front_len = strlen(front);
    size_t back_len = strlen(back);
    char *dst = malloc(front_len + back_len + sizeof('\0'));
    if (dst != NULL) {
        strncpy(&dst[0], front, front_len);
        strncpy(&dst[front_len], back, back_len);
        dst[front_len + back_len] = '\0';
    }

    return dst;
}

static size_t GetDistance(const ConfigStoreKvpHeader *p, const ConfigStoreKvpHeader *pEnd)
{
    return (ptrdiff_t)pEnd - (ptrdiff_t)p;
}

size_t ConfigStore_GetKvpFullSize(const ConfigStoreKvpHeader *p, const ConfigStoreKvpHeader *pEnd)
{
    if (!p) {
        return 0;
    }

    size_t avail_size = GetDistance(p, pEnd);

    if (p->size <= avail_size) {
        return p->size;
    } else {
        return avail_size;
    }
}

bool ConfigStore_CanDereferenceKvp(const ConfigStoreKvpHeader *p, const ConfigStoreKvpHeader *pEnd)
{
    bool ret = p && pEnd && GetDistance(p, pEnd) >= sizeof(*p) && (sizeof(*p) <= p->size) && (p->size <= GetDistance(p, pEnd));
    return ret;
}

ConfigStoreKvpHeader *ConfigStore_GetNextKvp(const ConfigStoreKvpHeader *p,
                                             const ConfigStoreKvpHeader *pEnd)
{
    size_t dist;
    if (!p) {
        dist = 0;
    } else if (ConfigStore_CanDereferenceKvp(p, pEnd)) {
        dist = p->size;
    } else {
        dist = GetDistance(p, pEnd);
    }

    ConfigStoreKvpHeader *retval = (ConfigStoreKvpHeader *)((ptrdiff_t)p + dist);
    if (!ConfigStore_CanDereferenceKvp(retval, pEnd)) {
        retval = (ConfigStoreKvpHeader *) pEnd;
    }

    return retval;
}

uint32_t ConfigStore_AddCrc(uint32_t init, const uint8_t *data, size_t size)
{
    uint32_t crc = init;
    const uint8_t *last = data + size;
    while (data != last) {
        crc = crc ^ *data++;
        for (int j = 7; j >= 0; --j) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return crc;
}

void ConfigStore_Init(ConfigStore *p)
{
    memset(p, 0, sizeof(*p));
    p->_fd = -1;
    printf("inside the ConfigStore_Init function\n");
    
    //DeleteAllTempFiles();
    printf("delete function is not called\n");
    printf("\n");
    printf("\n");
}

/*To delete the leftover tmp files on the device startup*/
void DeleteFileHelper(struct dirent *file, char* filePath)
{
    printf("\nhelper function\n");
    char *ptr, *fileName;
    int status=1;
    fileName=file->d_name;
    
    //rindex() is a string handling function that returns a pointer to the last occurrence 
    //of character c in string s, or a NULL pointer if c does not occur in the string.
    ptr = rindex(fileName, '.');

    //Check for filename extensions
    //if ((ptr != NULL) && ((strcmp(ptr, ".c") == 0)))
    if ((ptr != NULL) && ((strcmp(ptr, ".cfg") == 0)||(strcmp(ptr, ".conf") == 0)))
    { 
        char * str3 = (char *) malloc(1 + strlen(filePath)+ strlen(fileName) );
        strcpy(str3, filePath);
        strcat(str3, fileName);
        //delete the file
        status=unlink(str3);
        if(status !=0)
        {
            printf("\n temp file found but not able to delete it, file name is %s and the value of status is: %d",fileName,status);
            printf("\nthe errno value is: %d\n", errno);
            return;
        }
        else{
            printf("\ntemp file deleted\n");
	    printf("\nname of the deleted temp file: %s\n",fileName);
	}
        free(str3);  
    }
    else
    {
        printf("\n this is not a temp file, file name is %s: ",fileName);
        return;
    }
    //printf("\n able to delete the temp file");
    //return true;
    
}
void DeleteAllTempFiles()//(char *v[])
{
    printf("\ncalled from the normal world repo::\n");
    DIR *myDirectory;
    struct dirent *tempFile;

    if (true) 
    {
	printf("\ninside the true true function\n");
        //open the directory
        char input1[]="/home/hardikgarg/Microsoft/Bugs/114932/DeleteTempFiles/testFolder/";
        char input1[]="h:/mnt/config/7ba05ff7-7835-4b26-9eda-29af0c635280/";        
	myDirectory=opendir(input1);
        //myDirectory = opendir(v[1]);
        if (myDirectory) 
        {
            printf("the directory is opened, let's check its files:");
            //bool delete_res=false;
            while ((tempFile = readdir(myDirectory)))
            {
                DeleteFileHelper(tempFile,input1);
            }   
            // Close the directory
            if (closedir(myDirectory) == 0)
                printf("The directory is now closed.");
            else
                printf("The directory can not be closed.");
        }
	else{
	     printf("\nnot able to open the directory\n");
	} 
       
    } 

}



/*void deleteAllTempFiles(char *v[])
{
    DIR *myDirectory;
    struct dirent *tempFile;

    if (true) 
    {
        //open the directory
        char input1[]="/home/hardikgarg/Microsoft/Bugs/114932/DeleteTempFiles/testFolder/";
        
        //char* input2=input1;
        myDirectory=opendir(input1);
        //myDirectory = opendir(v[1]);
        if (myDirectory) 
        {
            printf("the directory is opened, let's check its files:");
            //bool delete_res=false;
            while ((tempFile = readdir(myDirectory)))
            {
                char * str3 = (char *) malloc(1 + strlen(input1)+ strlen(tempFile->d_name) );
                strcpy(str3, input1);
                strcat(str3, tempFile->d_name);
                printf("\nthe path is: %s\n",str3);
                //findTempFiles(tempFile);
                char *ptr, *fileName;
                int status=1;
                //rindex() is a string handling function that returns a pointer to the last occurrence 
                //of character c in string s, or a NULL pointer if c does not occur in the string.
            
                fileName=tempFile->d_name;
               
                
                //Check for filename extensions
                ptr = rindex(tempFile->d_name, '.');
                if ((ptr != NULL) && ((strcmp(ptr, ".c") == 0)))
                { 
                    //delete the file
                    printf("the file name is is is %s: ",tempFile->d_name);
                    status=unlink(str3);
                    if(status !=0)
                    {
                        printf("\n temp file found but not able to delete it, file name is %s and the value of status is: %d",fileName,status);
                        printf("\nthe errno value is: %d\n", errno);
                        //return;
                    }
                    else
                        printf("\ntemp file deleted");  
                }
                else
                {
                    printf("\n this is not a temp file, file name is %s: ",fileName);
                    //return;
                }
               
            }    
            // Close the directory
            if (closedir(myDirectory) == 0)
                printf("The directory is now closed.");
            else
                printf("The directory can not be closed.");
        } 
        
    } 

}*/

void ConfigStore_Close(ConfigStore *p)
{
    if (p->_fd >= 0) {
        close(p->_fd);
    }
    free(p->_primary_path);
    free(p->_replica_path);
    free(p->_begin);
    ConfigStore_Init(p);
}

void ConfigStore_Move(ConfigStore *pDst, ConfigStore *pSrc)
{
    if (pDst != pSrc) {
        ConfigStore_Close(pDst);
        memcpy(pDst, pSrc, sizeof(*pDst));
        ConfigStore_Init(pSrc);
    }
}

int ConfigStore_ReserveCapacity(ConfigStore *p, size_t capacity)
{
    if (capacity > p->_max_size) {
        // Can't grow the file beyond max size.
        errno = E2BIG;
        return -1;
    }

    size_t current_capacity = p->_capacity - p->_begin;

    if (capacity > current_capacity) {
        uint8_t *new_begin = realloc(p->_begin, capacity);
        if (new_begin == NULL) {
            return -1;
        }

        p->_capacity = &new_begin[capacity];
        p->_end = &new_begin[p->_end - p->_begin];
        p->_begin = new_begin;
    }

    return 0;
}

static bool ConfigStore_InvariantsCheck(const ConfigStore *p)
{
    bool ok = (p) && (p->_fd >= 0) && (p->_begin + sizeof(ConfigStoreFileHeader) <= p->_end) &&
              (p->_end <= p->_capacity);
    return ok;
}

static bool ReplicaTypeIsValid(ConfigStoreReplicaType rtype)
{
    switch (rtype) {
    case ConfigStoreReplica_None ... ConfigStoreReplica_Swap:
        return true;
    default:
        return false;
    }
}

static int Impl_Open(ConfigStore *p, const char *base_filepath, size_t max_size, int flags,
                     ConfigStoreReplicaType rtype)
{
    printf("inside the Imple_Open function");
    if (!ReplicaTypeIsValid(rtype)) {
        errno = EINVAL;
        return -1;
    }

    p->_replica_type = rtype;
    p->_max_size = max_size;

    p->_primary_path = strdup(base_filepath);
    if (p->_primary_path == NULL) {
        return -1;
    }
    printf("\nthe file name is h: %s\t\n",base_filepath);
    printf("check the line above");

    if (p->_replica_type == ConfigStoreReplica_Swap) {
        p->_replica_path = AppendString(base_filepath, ".tmp");
        if (p->_replica_path == NULL) {
            return -1;
        }

        // For swap mode, remove the swap file preemptively, even for readers.
        // If the swap exists on open, that means it's a leftover from a previous run that
        // crashed/exited before swaping it with the primary file.
        remove(p->_replica_path);
    }

    flags |= O_CLOEXEC;

    p->_fd = open(p->_primary_path, flags, S_IRUSR | S_IWUSR);
    if (p->_fd < 0) {
        return -1;
    }

    bool read_only = ((flags & (O_WRONLY | O_RDWR)) == 0);

    int lockmode = read_only ? (LOCK_SH | LOCK_NB) : (LOCK_EX | LOCK_NB);

    if (flock(p->_fd, lockmode) < 0) {
        return -1;
    }

    bool ok = false;
    off_t ssize = lseek(p->_fd, 0, SEEK_END);
    ok = (ssize >= 0) && (lseek(p->_fd, 0, SEEK_SET) == 0);
    if (!ok) {
        return -1;
    }

    size_t size = ssize;
    bool is_new = (size == 0);
    bool expects_new = (flags & (O_CREAT | O_TRUNC));

    if (is_new) {
        if (!expects_new) {
            errno = ENOENT;
            return -1;
        }
        size = sizeof(ConfigStoreFileHeader);
    }

    if (size < sizeof(ConfigStoreFileHeader)) {
        errno = ERANGE;
        return -1;
    }

    if (ConfigStore_ReserveCapacity(p, size)) {
        return -1;
    }

    ConfigStoreFileHeader *header = (ConfigStoreFileHeader *)(p->_begin);

    if (is_new) {
        // For new files, start with a basic header.
        header->header.size = sizeof(ConfigStoreFileHeader);
        header->header.key = ConfigStoreFileHeaderKey;
        header->signature = ConfigStoreFileSignature;
        header->version = ConfigStoreFileVersion;
        p->_end += sizeof(ConfigStoreFileHeader);
    } else {
        // For existing files, try to read the store from them.
        if (read(p->_fd, p->_begin, size) != ssize) {
            return -1;
        }

        size_t content_size = ConfigStore_ValidateFormat(p->_begin, size);
        if (content_size == 0) {
            // Invalid content.
            errno = EINVAL;
            return -1;
        }

        bool must_truncate =
            !read_only && (content_size < size) && (p->_replica_type != ConfigStoreReplica_Swap);

        if (must_truncate) {
            // The content is valid, but it's shorter than the file. The previous writer may have
            // crashed after it wrote the content but before it truncated the file, so truncate it
            // now.

            if (ftruncate(p->_fd, content_size) != 0) {
                return -1;
            }

            fsync(p->_fd);
        }

        p->_end += content_size;
    }

    return 0;
}

int ConfigStore_StatVfs(const char *path, struct statvfs *buf)
{
    return statvfs(path, buf);
}

/// <summary>
/// Adjusts the max file size considering an overhead per storage block, which the file system may
/// consume for pointers and other metadata.
/// </summary>
/// <returns> The adjust size on success, or zero on failure. </summary>
static size_t AdjustedMaxFileSize(const char *file_path, size_t file_size)
{
    if (file_size <= ConfigStoreOverheadPerStorageBlock) {
        return 0;
    }

    char *path_copy = strdup(file_path);
    if (path_copy == NULL) {
        return 0;
    }

    // Get block size
    struct statvfs stat_buf;
    int r = ConfigStore_StatVfs(dirname(path_copy), &stat_buf);
    free(path_copy);

    if (r) {
        return 0;
    }

    const size_t BlockSize = stat_buf.f_bsize;

    size_t pointer_overhead =
        ((file_size - 1) / BlockSize + 1) * ConfigStoreOverheadPerStorageBlock;

    return file_size - pointer_overhead;
}

int ConfigStore_Open(ConfigStore *p, const char *base_filepath, size_t max_size, int flags,
                     ConfigStoreReplicaType rtype)
{
    if (p->_fd >= 0) {
        errno = EALREADY;
        return -1;
    }

    ConfigStore temp;
    ConfigStore_Init(&temp);

    size_t adjusted_max_size = AdjustedMaxFileSize(base_filepath, max_size);
    if (adjusted_max_size == 0) {
        errno = ENOSPC;
        return -1;
    }

    int res = Impl_Open(&temp, base_filepath, adjusted_max_size, flags, rtype);

    if (res == 0) {
        ConfigStore_Move(p, &temp);
    }

    ConfigStore_Close(&temp);

    return res;
}

static int Impl_WriteToFile(int fd, ConfigStore *p)
{
    if (lseek(fd, 0, SEEK_SET) < 0) {
        return -1;
    }

    ssize_t total_size = p->_end - p->_begin;

    if (write(fd, p->_begin, total_size) != total_size) {
        return -1;
    }

    if (ftruncate(fd, total_size) != 0) {
        return -1;
    }

    fsync(fd);

    return 0;
}

int ConfigStore_Commit(ConfigStore *p)
{
    if (!ConfigStore_InvariantsCheck(p)) {
        errno = EINVAL;
        return -1;
    }

    uint32_t crc =
        ConfigStore_AddCrc(ConfigStoreCrcInitValue, p->_begin + sizeof(ConfigStoreFileHeader),
                           p->_end - p->_begin - sizeof(ConfigStoreFileHeader));

    ConfigStoreKvpHeader *first = (ConfigStoreKvpHeader *)p->_begin;
    ConfigStoreKvpHeader *last = (ConfigStoreKvpHeader *)p->_end;

    if ((first != last) && (first->key == ConfigStoreFileHeaderKey)) {
        ConfigStoreFileHeader *header = (ConfigStoreFileHeader *)(first);
        header->file_size = (p->_end - p->_begin);
        header->crc = crc;
    }

    if (p->_replica_type == ConfigStoreReplica_Swap) {
        // Create the swap file always.
        int fd = open(p->_replica_path, O_RDWR | O_CREAT | O_CLOEXEC | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            return -1;
        }
        int res = Impl_WriteToFile(fd, p);
        close(fd);
        if (res < 0) {
            return -1;
        }
        res = rename(p->_replica_path, p->_primary_path);
        if (res < 0) {
            return -1;
        }

        ConfigStore_Close(p);
    } else {
        return Impl_WriteToFile(p->_fd, p);
    }

    return 0;
}

ConfigStoreKvpHeader *ConfigStore_BeginKvp(const ConfigStore *p)
{
    return ConfigStore_GetNextKvp((ConfigStoreKvpHeader *)p->_begin,
                                  (ConfigStoreKvpHeader *)p->_end);
}

ConfigStoreKvpHeader *ConfigStore_EndKvp(const ConfigStore *p)
{
    return (ConfigStoreKvpHeader *)p->_end;
}

ConfigStoreKvpHeader *ConfigStore_InsertKvp(ConfigStore *p, const ConfigStoreKvpHeader *pos,
                                            ConfigStoreKey key, size_t size)
{
    uint16_t kvp_size;
    if (__builtin_add_overflow(size, sizeof(ConfigStoreKvpHeader), &kvp_size)) {
        return NULL;
    }

    size_t in_offset = (ptrdiff_t)pos - (ptrdiff_t)p->_begin;
    size_t current_size = p->_end - p->_begin;

    if (ConfigStore_ReserveCapacity(p, current_size + kvp_size)) {
        return NULL;
    }

    uint8_t *in_pos = &p->_begin[in_offset];

    memmove(&in_pos[kvp_size], in_pos, current_size - in_offset);

    ConfigStoreKvpHeader *pKvp = (ConfigStoreKvpHeader *)(in_pos);
    pKvp->size = kvp_size;
    pKvp->key = key;

    p->_end += kvp_size;

    return pKvp;
}

static ConfigStoreKvpHeader *Impl_FindKey(ConfigStoreKey key, ConfigStoreKvpHeader *pFirst,
                                          ConfigStoreKvpHeader *pLast)
{
    while ((pFirst != pLast) && (pFirst->key != key)) {
        pFirst = ConfigStore_GetNextKvp(pFirst, pLast);
    }

    return pFirst;
}

ConfigStoreKvpHeader *ConfigStore_TryGetKey(const ConfigStore *p, ConfigStoreKey key)
{
    ConfigStoreKvpHeader *it = ConfigStore_BeginKvp(p);
    ConfigStoreKvpHeader *it_end = ConfigStore_EndKvp(p);
    it = Impl_FindKey(key, it, it_end);
    return (it != it_end) ? it : NULL;
}

ConfigStoreKvpHeader *ConfigStore_PutUniqueKey(ConfigStore *p, ConfigStoreKey key,
                                               const uint8_t *optional_data, size_t value_size)
{
    ConfigStoreKvpHeader *it = ConfigStore_BeginKvp(p);
    ConfigStoreKvpHeader *it_end = NULL;

    // For all matching keys.
    while (it_end = ConfigStore_EndKvp(p), it = Impl_FindKey(key, it, it_end), it != it_end) {
        if (it->size != value_size) {
            // Not same size. Erase KVP and continue with next.
            it = ConfigStore_EraseKvp(p, it);
            continue;
        }

        // Found KVP with same size. Reuse it and erase any other occurrences of the same
        // key after it, just in case.
        ConfigStoreKvpHeader *it_erase = ConfigStore_GetNextKvp(it, it_end);
        while (it_end = ConfigStore_EndKvp(p), it_erase = Impl_FindKey(key, it_erase, it_end),
               it_erase != it_end) {
            it_erase = ConfigStore_EraseKvp(p, it_erase);
        }
        break;
    }

    it_end = ConfigStore_EndKvp(p);
    if (it == it_end) {
        it = ConfigStore_InsertKvp(p, it_end, key, value_size);
        if (it == ConfigStore_EndKvp(p)) {
            // Space exhaustion.
            return NULL;
        }
    }

    if (optional_data != NULL) {
        ConfigStore_WriteValue(it, 0, optional_data, value_size);
    }

    return it;
}

ConfigStoreKvpHeader *ConfigStore_EraseKvp(ConfigStore *p, const ConfigStoreKvpHeader *pos)
{
    size_t size = pos->size;
    ptrdiff_t offset = (ptrdiff_t)pos - (ptrdiff_t)p->_begin;
    uint8_t *out_pos = &p->_begin[offset];
    memmove(&out_pos[0], &out_pos[size], p->_end - &out_pos[size]);
    p->_end -= size;

    return (ConfigStoreKvpHeader *)out_pos;
}

ConfigStoreKvpHeader *ConfigStore_AllocUniqueKvp(ConfigStore *p, ConfigStoreKey first_key,
                                                 ConfigStoreKey last_key, size_t value_size,
                                                 ConfigStoreKey key_increment)
{
    while (first_key < last_key) {

        bool found = false;

        ConfigStoreKvpHeader *kvp = ConfigStore_BeginKvp(p);
        while (kvp != ConfigStore_EndKvp(p)) {
            found = (kvp->key == first_key);
            if (found) {
                break;
            }
            kvp = ConfigStore_GetNextKvp(kvp, ConfigStore_EndKvp(p));
        }

        if (!found) {
            break;
        }

        if (__builtin_add_overflow(first_key, key_increment, &first_key)) {
            errno = ENOENT;
            return NULL;
        }
    }

    if (first_key >= last_key) {
        errno = ENOENT;
        return NULL;
    }

    return ConfigStore_InsertKvp(p, ConfigStore_EndKvp(p), first_key, value_size);
}

int ConfigStore_EraseKeysInRange(ConfigStore *p, ConfigStoreKey first_key, ConfigStoreKey last_key,
                                 ConfigStoreKey key_increment)
{
    bool good_args = (p) && (first_key <= last_key) && (1 <= key_increment);
    if (!good_args) {
        errno = EINVAL;
        return -1;
    }

    ConfigStoreKvpHeader *kvp = ConfigStore_BeginKvp(p);
    while (kvp != ConfigStore_EndKvp(p)) {
        bool match = (first_key <= kvp->key) && (kvp->key < last_key) &&
                     (((kvp->key - first_key) % key_increment) == 0);
        if (match) {
            kvp = ConfigStore_EraseKvp(p, kvp);
        } else {
            kvp = ConfigStore_GetNextKvp(kvp, ConfigStore_EndKvp(p));
        }
    }

    return 0;
}

ConfigStoreKvpHeader *ConfigStore_GetNextKvpInRange(ConfigStore *p, const ConfigStoreKvpHeader *pos,
                                                    ConfigStoreKey first_key,
                                                    ConfigStoreKey last_key,
                                                    ConfigStoreKey key_increment)
{
    ConfigStoreKvpHeader *end_pos = ConfigStore_EndKvp(p);

    pos = pos ? ConfigStore_GetNextKvp(pos, end_pos) : ConfigStore_BeginKvp(p);

    while (pos != end_pos) {
        bool match = (first_key <= pos->key) && (pos->key < last_key) &&
                     (((pos->key - first_key) % key_increment) == 0);
        if (match) {
            break;
        }
        pos = ConfigStore_GetNextKvp(pos, end_pos);
    }

    return (ConfigStoreKvpHeader *)pos;
}

int ConfigStore_WriteValue(ConfigStoreKvpHeader *pos, size_t offset, const void *data, size_t size)
{
    size_t hdr_size = pos ? sizeof(*pos) : 0;
    size_t dst_size = (pos && (pos->size > sizeof(*pos))) ? (pos->size - sizeof(*pos)) : 0;

    uint8_t *dst_data = (uint8_t *)pos + hdr_size + offset;

    size_t last_offset = offset + size;
    if (dst_size < last_offset) {
        errno = E2BIG;
        return -1;
    }

    memcpy(dst_data, (uint8_t *)data, size);
    memset(dst_data + last_offset, 0, dst_size - last_offset);

    return 0;
}

size_t ConfigStore_ValidateFormat(const uint8_t *data, size_t size)
{
    const ConfigStoreKvpHeader *first = (const ConfigStoreKvpHeader *)data;
    const ConfigStoreKvpHeader *last = (const ConfigStoreKvpHeader *)(data + size);

    bool has_header = (first != NULL) && (first != last) &&
                      (first->key == ConfigStoreFileHeaderKey) &&
                      (first->size >= sizeof(ConfigStoreFileHeader));

    if (!has_header) {
        return 0;
    }

    const ConfigStoreFileHeader *header = (const ConfigStoreFileHeader *)first;

    bool ok = (header->signature == ConfigStoreFileSignature) &&
              (header->version == ConfigStoreFileVersion) &&
              (header->header.size <= header->file_size) && (header->file_size <= size);
    if (!ok) {
        return 0;
    }

    size = header->file_size;

    data += sizeof(ConfigStoreFileHeader);
    size -= sizeof(ConfigStoreFileHeader);

    uint32_t crc = ConfigStore_AddCrc(ConfigStoreCrcInitValue, data, size);

    if (crc != header->crc) {
        return 0;
    }

    ++first;

    while ((first != NULL) && (first != last)) {
        if (first->key == ConfigStoreFileHeaderKey) {
            // The header key must only be used in the beginning of the file.
            break;
        }

        first = ConfigStore_GetNextKvp(first, last);
    }

    if (first != last) {
        // Didn't get to the end of the file.
        return 0;
    }

    return header->file_size;
}
