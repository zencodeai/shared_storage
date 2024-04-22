#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdexcept>
#include <string>

#include "sqlite3.h"

#define LOG_ENABLED

#ifdef LOG_ENABLED
#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, "SST", __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "SST", __VA_ARGS__)
#else
#define LOG_DEBUG(...) do {} while(0)
#define LOG_ERROR(...) do {} while(0)
#endif


// JNI String class
class CJNIString {

private:

    JNIEnv *m_env;
    jstring m_jstr;
    const char *m_str;

public:

    CJNIString(JNIEnv *env, jstring jstr) : m_env(env), m_jstr(jstr) {
        m_str = env->GetStringUTFChars(jstr, nullptr);
    }

    ~CJNIString() {

        // Release the string
        if (m_str != nullptr) {
            m_env->ReleaseStringUTFChars(m_jstr, m_str);
        }
    }

    const char *c_str() const {

        // Throw exception if the string is null
        if (m_str == nullptr) {
            throw std::runtime_error("String is null");
        }

        return m_str;
    }
};

// SQLite3 database wrapper
class CDatabase {

private:

    sqlite3 *m_db;

    // Create database
    void createDatabase(const char* path) {

        try {
            LOG_DEBUG("Creating database: %s", path);

            // Create database file
            int rc = sqlite3_open(path, &m_db);

            // Change the permissions of the database file
            chmod(path, 0666);

            // Throw exception if the database could not be created
            if (rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }

            // Create values table (name, value)
            const char* sql = "CREATE TABLE IF NOT EXISTS KEYS (KEY TEXT PRIMARY KEY, VALUE TEXT);";
            char *errMsg = nullptr;
            rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);

            // Throw exception if the table could not be created
            if (rc != SQLITE_OK) {
                throw std::runtime_error(errMsg);
            }
        }
        catch (const std::exception &e) {

            // Close the database
            close();

            // Remove the database file
            unlink(path);

            // Throw the exception
            throw;
        }
    }

public:

    // ctor
    CDatabase() : m_db(nullptr) {}

    // Close the database if it is open
    void close() {

        LOG_DEBUG("Closing database");
        if (m_db != nullptr) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    // Open/create database
    void open(const char* path) {

        LOG_DEBUG("Opening database: %s", path);

        // Close the database if it is open
        close();

        // Does database file exist?
        if (access(path, F_OK) != 0) {

            LOG_DEBUG("Database file does not exist: %s", path);

            // Create the database
            createDatabase(path);
        }
        else {

            LOG_DEBUG("Database file exists: %s", path);

            // Open the database
            int rc = sqlite3_open(path, &m_db);

            // Throw exception if the database could not be opened
            if (rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(m_db));
            }
        }
    }

    // Set value
    void setValue(const char* key, const char* value) {

        LOG_DEBUG("Setting value: %s=%s", key, value);

        // Insert or replace the key and value
        const char* sql = "INSERT OR REPLACE INTO KEYS (KEY, VALUE) VALUES (?, ?);";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

        // Throw exception if the statement could not be prepared
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Bind the key
        rc = sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

        // Throw exception if the key could not be bound
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Bind the value
        rc = sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);

        // Throw exception if the value could not be bound
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Execute the statement
        rc = sqlite3_step(stmt);

        // Throw exception if the statement could not be executed
        if (rc != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Finalize the statement
        rc = sqlite3_finalize(stmt);

        // Throw exception if the statement could not be finalized
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }
    }

    // Get value
    std::string getValue(const char* key) {

        LOG_DEBUG("Getting value: %s", key);

        // Select the value
        const char* sql = "SELECT VALUE FROM KEYS WHERE KEY = ?;";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

        // Throw exception if the statement could not be prepared
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Bind the key
        rc = sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

        // Throw exception if the key could not be bound
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Execute the statement
        rc = sqlite3_step(stmt);

        // Throw exception if the statement could not be executed
        if (rc != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        // Get the value
        std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        LOG_DEBUG("Value: %s", value.c_str());

        // Finalize the statement
        rc = sqlite3_finalize(stmt);

        // Throw exception if the statement could not be finalized
        if (rc != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(m_db));
        }

        return value;
    }

    // dtor
    ~CDatabase() {

        // Close the database
        close();
    }
};


// Database instance
CDatabase g_database;


// JNI function for private external fun sstCreate(localDataPath: String, externalDataPath: String): Unit
// In package: com.example.shared_storage
// And activity: MainActivity
extern "C" JNIEXPORT void JNICALL
Java_com_example_shared_1storage_MainActivity_sstCreate(JNIEnv *env, jobject thiz, jstring localDataPath, jstring externalDataPath) {

    try {

        CJNIString localDataPathStr(env, localDataPath);
        CJNIString externalDataPathStr(env, externalDataPath);

        // Log the paths
        LOG_DEBUG("Local data path: %s", localDataPathStr.c_str());
        LOG_DEBUG("External data path: %s", externalDataPathStr.c_str());

        // Build the database path
        std::string dbPath = externalDataPathStr.c_str();
        dbPath += "/sst_app.db";

        // Open the database
        g_database.open(dbPath.c_str());
    }
    catch (const std::exception &e) {

        // Log the exception
        LOG_ERROR("Exception: %s", e.what());
        // Throw the exception
        env->ThrowNew(env->FindClass("java/lang/Exception"), e.what());
    }
}

// JNI function for private external fun sstSet(key: String, value: String): Unit
// In package: com.example.shared_storage
// And activity: MainActivity
extern "C" JNIEXPORT void JNICALL
Java_com_example_shared_1storage_MainActivity_sstSet(JNIEnv *env, jobject thiz, jstring key, jstring value) {

    try {

        CJNIString keyStr(env, key);
        CJNIString valueStr(env, value);

        // Log the key and value
        LOG_DEBUG("Key: %s", keyStr.c_str());
        LOG_DEBUG("Value: %s", valueStr.c_str());

        // Set the value
        g_database.setValue(keyStr.c_str(), valueStr.c_str());
    }
    catch (const std::exception &e) {

        // Log the exception
        LOG_ERROR("Exception: %s", e.what());
        // Throw the exception
        env->ThrowNew(env->FindClass("java/lang/Exception"), e.what());
    }
}

// JNI function for private external fun sstGet(key: String): String
// In package: com.example.shared_storage
// And activity: MainActivity
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_shared_1storage_MainActivity_sstGet(JNIEnv *env, jobject thiz, jstring key) {

    try {

        CJNIString keyStr(env, key);

        // Log the key
        LOG_DEBUG("Key: %s", keyStr.c_str());

        // Get the value
        std::string value = g_database.getValue(keyStr.c_str());
        LOG_DEBUG("Value: %s", value.c_str());

        // Return the value
        jstring jvalue = env->NewStringUTF(value.c_str());

        // throw exception if the value is null
        if (jvalue == nullptr) {
            throw std::runtime_error("Value is null");
        }

        return jvalue;
    }
    catch (const std::exception &e) {

        // Log the exception
        LOG_ERROR("Exception: %s", e.what());
        // Throw the exception
        env->ThrowNew(env->FindClass("java/lang/Exception"), e.what());
        return nullptr;
    }
}

// JNI function for private external fun sstClose(): Unit
// In package: com.example.shared_storage
// And activity: MainActivity
extern "C" JNIEXPORT void JNICALL
Java_com_example_shared_1storage_MainActivity_sstClose(JNIEnv *env, jobject thiz) {

    try {

        // Close the database
        g_database.close();
    }
    catch (const std::exception &e) {

        // Log the exception
        LOG_ERROR("Exception: %s", e.what());
        // Throw the exception
        env->ThrowNew(env->FindClass("java/lang/Exception"), e.what());
    }
}
