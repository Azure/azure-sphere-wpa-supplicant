#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary> The key type. </summary>
typedef uint16_t ConfigStoreKey;

/// <summary> The serialized header of a key-value pair. </summary>
typedef struct ConfigStoreKvpHeader {
    ConfigStoreKey key; // The key of this KVP.
    uint16_t size;      // The size of the key-value pair (including this header).
} __attribute__((packed)) ConfigStoreKvpHeader;

/// <summary> The serialized header of the store file. </summary>
typedef struct ConfigStoreFileHeader {
    ConfigStoreKvpHeader header; // Header
    uint8_t signature;           // File signature.
    uint8_t version;             // File version.
    uint32_t file_size;          // The size of the file (including this header).
    uint32_t crc;                // The CRC of the portion of the file after this field.
} __attribute__((packed)) ConfigStoreFileHeader;

/// <summary> Range of keys reserved for the store itself. </summary>
static const uint16_t ConfigStoreMinKey = 0x0000;
static const uint16_t ConfigStoreMaxKey = 0xFFFA;
static const uint16_t ConfigStoreMinReservedKey = 0xFFFB;
static const uint16_t ConfigStoreMaxReservedKey = 0xFFFF;
static const uint16_t ConfigStoreInvalidKey = 0xFFFF;
static const uint16_t ConfigStoreFileHeaderKey = 0xFFFB;
static const uint32_t ConfigStoreCrcInitValue = 0xFFFFFFFF;

static const uint8_t ConfigStoreFileSignature = 0xC6;
static const uint8_t ConfigStoreFileVersion = 0;

/// <summary>
/// This adjusts the file system overhead for each storage block.
/// The file system consumes some bytes of the block to store pointers and other metadata.
/// </summary>
static const size_t ConfigStoreOverheadPerStorageBlock = 16;

/// <summary>
/// The type of replica to use.
/// </summary>
typedef enum ConfigStoreReplicaType {
    /// <summary> Don't use replicas. The store file is overwritten in place. </summary>
    ConfigStoreReplica_None = 0,
    /// <summary> Use a swap file. The file is swapped atomically with a temp file. </summary>
    ConfigStoreReplica_Swap = 1,
} ConfigStoreReplicaType;

/// <summary> Gets the full size of the KVP given the header. </summary>
/// <returns> The full size of the KVP, or 0 if the KVP is invalid. </returns>
size_t ConfigStore_GetKvpFullSize(const ConfigStoreKvpHeader *p, const ConfigStoreKvpHeader *pEnd);

/// <summary> Checks if the KVP header is defined within the given size. </summary>
/// <returns> true if the KVP can be dereferenced within the range; false otherwise. </summary>
bool ConfigStore_CanDereferenceKvp(const ConfigStoreKvpHeader *p, const ConfigStoreKvpHeader *pEnd);

/// <summary> Increments the pointer to the next KVP given the current and the "guard". </summary>
/// <returns> The next KVP or <paramref name="pEnd" /> if at the end of the range. </summary>
ConfigStoreKvpHeader *ConfigStore_GetNextKvp(const ConfigStoreKvpHeader *p,
                                             const ConfigStoreKvpHeader *pEnd);

/// <summary> The Config Store State. </summary>
typedef struct ConfigStore {
    int _fd;
    uint8_t *_begin;
    uint8_t *_end;
    uint8_t *_capacity;
    size_t _max_size;
    ConfigStoreReplicaType _replica_type;
    char *_primary_path;
    char *_replica_path;
} ConfigStore;

/// <summary>
/// Initializes the memory of a ConfigStore for usage. Equivalent to the constructor.
/// </summary>
void ConfigStore_Init(ConfigStore *p);

/// <summary>
/// Resets the memory of a ConfigStore. Disposes of any allocated resources. Equivalent to a
/// destructor, but puts the store back into an initialized state.
/// </summary>
void ConfigStore_Close(ConfigStore *p);

/// <summary>
/// Transfers the resources of a ConfigStore to another.
/// </summary>
void ConfigStore_Move(ConfigStore *pDst, ConfigStore *pSrc);

/// <summary>
/// Reserves space to add one or more KVP. It's not necessary to call this before
/// inserting a key, but it can help reduce memory fragmentation and re-allocation to pre-reserve
/// space.
/// </summary>
/// <returns> 0 on success; -1 on failure with error indication in errno. </returns>
int ConfigStore_ReserveCapacity(ConfigStore *p, size_t capacity);

/// <summary>
/// Opens the store for writing. If the file doesn't exist, the function creates one anew.
/// </summary>
/// <returns> 0 on success; -1 on failure with error indication in errno. </returns>
int ConfigStore_Open(ConfigStore *p, const char *base_filepath, size_t max_size, int flags,
                     ConfigStoreReplicaType rtype);

/// <summary>
/// Commits the in-memory changes back to persistent storage.
/// Note:
/// If the file was opened in ConfigStoreReplica_Swap replica mode, this call will also close the
/// object. This is because the object can't re-acquire its lock on the file without re-opening it,
/// which temporarily allows for other objects to open and lock it. In this case the object may as
/// well close the file on commit.
/// </summary>
/// <returns> 0 on success; -1 on failure with error indication in errno. </returns>
int ConfigStore_Commit(ConfigStore *p);

/// <summary> Gets a pointer to the first KVP in the store. </summary>
/// <param name="p"> Required pointer to the store. </param>
/// <returns> A pointer for the KVP. </returns>
ConfigStoreKvpHeader *ConfigStore_BeginKvp(const ConfigStore *p);

/// <summary> Gets a pointer to the "guard" KVP of the store. </summary>
/// <returns> A pointer for the guard KVP. </returns>
ConfigStoreKvpHeader *ConfigStore_EndKvp(const ConfigStore *p);

/// <summary> Inserts a KVP of a given size and at a given position. </summary>
/// <returns> A pointer for the inserted KVP or the guard KVP on memory exhaustion. </returns>
ConfigStoreKvpHeader *ConfigStore_InsertKvp(ConfigStore *p, const ConfigStoreKvpHeader *pos,
                                            ConfigStoreKey key, size_t size);

/// <summary> Erases a KVP in a given position. </summary>
/// <returns> A pointer for the KVP following the one that was removed. </returns>
ConfigStoreKvpHeader *ConfigStore_EraseKvp(ConfigStore *p, const ConfigStoreKvpHeader *pos);

/// <summary>
/// Allocates a KVP with a key that is unique in a given range and key-increment.
/// </summary>
/// <param name="first_key"> The first key in the searchable range. </param>
/// <param name="last_key"> The last key (exclusive) in the searchable range. </param>
/// <param name="value_size"> The size to reserve for the value. </param>
/// <param name="key_increment"> The increment for each step. </param>
/// <returns> The new KVP with a unique key or nullptr on failure. </returns>
/// <remarks>
/// On success, the key of the returned KVP will have the following invariants.
///     first_key <= kvp->key() < last_key
///     (kvp->key() - first_key) % key_increment == 0
///     kvp->size() == value_size
/// </remarks>
/// <returns>
/// Pointer to the inserted KVP on success; NULL on failure with error indication in errno.
/// - ENOENT: no unique key could be allocated in the specified range.
/// - ENOMEM: no space available to insert the KVP.
/// </returns>
ConfigStoreKvpHeader *ConfigStore_AllocUniqueKvp(ConfigStore *p, ConfigStoreKey first_key,
                                                 ConfigStoreKey last_key, size_t value_size,
                                                 ConfigStoreKey key_increment);

/// <summary>
/// Erases all KVP that match a key in the given range.
/// Note the end of the range is **EXCLUSIVE**.
/// </summary>
/// <param name="first_key"> The first key in the range. </param>
/// <param name="last_key"> The last key (exclusive) in the range. </param>
/// <param name="key_increment"> The increment for each step. </param>
/// <returns> 0 on success; -1 on failure with error indication in errno. </returns>
int ConfigStore_EraseKeysInRange(ConfigStore *p, ConfigStoreKey first_key, ConfigStoreKey last_key,
                                 ConfigStoreKey key_increment);

/// <summary>
/// Gets the next KVP in a range of keys.
/// Note the end of the range is **EXCLUSIVE**.
/// </summary>
/// <param name="p"> The store. </param>
/// <param name="pos"> The current KVP. If null, will return the first that matches the key range.
/// </param>
/// <param name="first_key"> The first key in the range. </param>
/// <param name="last_key"> The last key (exclusive) in the range. </param>
/// <param name="key_increment"> The increment for each step. </param>
/// <returns> The next KVP that matches the criteria or ConfigStore_EndKvp at the end. </returns>
ConfigStoreKvpHeader *ConfigStore_GetNextKvpInRange(ConfigStore *p, const ConfigStoreKvpHeader *pos,
                                                    ConfigStoreKey first_key,
                                                    ConfigStoreKey last_key,
                                                    ConfigStoreKey key_increment);

/// <summary> Attempts to get the first match of a key. </summary>
/// <returns> Pointer to the KVP or null if the key is not found. </returns>
ConfigStoreKvpHeader *ConfigStore_TryGetKey(const ConfigStore *p, ConfigStoreKey key);

/// <summary>
/// Puts a KVP in the store and ensures its key is unique by erasing any other KVP of same key.
/// Optionally the function also copies a value to the KVP's value.
/// </summary>
ConfigStoreKvpHeader *ConfigStore_PutUniqueKey(ConfigStore *p, ConfigStoreKey key,
                                               const uint8_t *optional_data, size_t value_size);

/// <summary> Helper to write to a value of a KVP. </summary>
/// <returns> 0 on success; -1 on failure with error indication in errno. </returns>
int ConfigStore_WriteValue(ConfigStoreKvpHeader *pos, size_t offset, const void *data, size_t size);

/// <summary> Checks if the contents of a buffer are a valid configuration store. </summary>
/// <returns> 0 if the contents are invalid; the valid size if the contents are valid. </returns>
size_t ConfigStore_ValidateFormat(const uint8_t *data, size_t size);

/// <summary> Helper to compute CRC. </summary>
/// <returns> The CRC value. </returns>
uint32_t ConfigStore_AddCrc(uint32_t init, const uint8_t *data, size_t size);

/// <summary>
/// Helper to get stats for the file system.
/// This helper is linked as a weak symbol so it can be overwrited by the target for testing.
/// </summary>
struct statvfs;
extern int ConfigStore_StatVfs(const char *path, struct statvfs *buf) __attribute__((weak));

#ifdef __cplusplus
}
#endif