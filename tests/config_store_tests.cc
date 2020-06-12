#include <config_store.h>

#include <ftw.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <malloc.h>

namespace config
{

class ConfigStoreTests : public testing::Test
{
public:
    static constexpr char TempTestDir[] = P_tmpdir "/config-store-tests";
    static constexpr size_t AnyMaxSize = 8 * 1024;

    static void SetUpTestCase()
    {
        RemoveTestTempDir();
        int r = mkdir(TempTestDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        ASSERT_TRUE(r == 0 || errno == EEXIST) << errno;
        chdir(TempTestDir);
        SetUpFilesInDir();
    }

    //to test the ConfigStore_DeleteAllTempFiles function
    static void SetUpFilesInDir()
    {
        //create temp files
        char filePath[40];
        for(size_t i=0;i<5;++i)
        {
            snprintf(filePath, 40, "%s/TestFile%d.tmp", TempTestDir,i);
            FILE* filePtr = fopen(filePath,"w");
            
            //check if the file is created
            ASSERT_TRUE(filePtr != 0 || errno == EEXIST) << errno;
            int fileClosePtr= fclose(filePtr);
            //check if the file is closed
            ASSERT_TRUE(fileClosePtr == 0 || errno == EEXIST) << errno;
        }
    }

    static void TearDownTestCase() { RemoveTestTempDir(); }

    static void RemoveTestTempDir()
    {
        auto cb = [](const char *fpath, const struct stat *, int, struct FTW *) -> int {
            EXPECT_EQ(remove(fpath), 0) << errno;
            return 0;
        };

        nftw(TempTestDir, cb, 64, FTW_DEPTH | FTW_PHYS);
    }

    static std::string GetCurrentTestName()
    {
        return ::testing::UnitTest::GetInstance()->current_test_info()->name();
    }
};

TEST_F(ConfigStoreTests, DeleteTempFile)
{
    ConfigStore_DeleteAllTempFiles((char*)TempTestDir);
    DIR *myDirectory;
    struct dirent *fileName;

    bool res=false;
    //open the directory
    myDirectory = opendir(TempTestDir);
    //inside the directory
    if (myDirectory)
    {
        //read the files in the directory
        while ((fileName = readdir(myDirectory)))
        {
            char *ptr;
            ptr = rindex(fileName->d_name, '.');

            //Check for filename extensions
            if ((ptr != NULL) && (strncmp(ptr, ".tmp", 4) == 0))
            {
                res=true;
            }
        }
        ASSERT_TRUE(res == false || errno == EEXIST) << errno;
        // Close the directory
        closedir(myDirectory);
    }
    
}
TEST_F(ConfigStoreTests, WriterCanCreateFile)
{
    auto file_name = GetCurrentTestName();

    struct stat st;
    ASSERT_EQ(::stat(file_name.c_str(), &st), -1);
    ASSERT_EQ(errno, ENOENT);

    ConfigStore sto;
    ConfigStore_Init(&sto);

    ASSERT_EQ(ConfigStore_Open(&sto, file_name.c_str(), AnyMaxSize, O_RDWR | O_CREAT | O_CLOEXEC,
                               ConfigStoreReplica_None),
              0)
        << errno;

    // Empty because it hasn't been committed yet.
    ASSERT_EQ(::stat(file_name.c_str(), &st), 0);
    ASSERT_EQ(st.st_size, 0);

    ASSERT_EQ(ConfigStore_Commit(&sto), 0) << errno;

    ASSERT_EQ(::stat(file_name.c_str(), &st), 0);
    ASSERT_EQ(st.st_size, sizeof(ConfigStoreFileHeader));

    ConfigStore_Close(&sto);
}

TEST_F(ConfigStoreTests, WriterCanAddEntryToFile)
{
    auto file_name = GetCurrentTestName();

    ConfigStore sto;
    ConfigStore_Init(&sto);

    ASSERT_EQ(ConfigStore_Open(&sto, file_name.c_str(), AnyMaxSize, O_RDWR | O_CREAT | O_CLOEXEC,
                               ConfigStoreReplica_None),
              0)
        << errno;

    ASSERT_EQ(ConfigStore_BeginKvp(&sto), ConfigStore_EndKvp(&sto));

    constexpr ConfigStoreKey AnyKey = 189;
    constexpr uint8_t AnyData[] = {0x94, 0xa9, 0xbe, 0xb0, 0x57, 0xe7, 0x71, 0xee, 0x1e};

    auto it = ConfigStore_InsertKvp(&sto, ConfigStore_EndKvp(&sto), AnyKey, sizeof(AnyData));

    ASSERT_EQ(it, ConfigStore_BeginKvp(&sto));
    ASSERT_NE(it, ConfigStore_EndKvp(&sto));
    ASSERT_EQ(it->key, AnyKey);
    ASSERT_EQ(it->size, sizeof(*it) + sizeof(AnyData));

    it = ConfigStore_GetNextKvp(it, ConfigStore_EndKvp(&sto));
    ASSERT_EQ(it, ConfigStore_EndKvp(&sto));

    ASSERT_EQ(ConfigStore_Commit(&sto), 0) << errno;

    struct stat st;
    ASSERT_EQ(::stat(file_name.c_str(), &st), 0);
    ASSERT_EQ(st.st_size,
              sizeof(ConfigStoreFileHeader) + sizeof(ConfigStoreKvpHeader) + sizeof(AnyData));

    ConfigStore_Close(&sto);
}

} // namespace config
