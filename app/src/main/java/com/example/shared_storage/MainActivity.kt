package com.example.shared_storage

import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.widget.Button
import android.widget.EditText
import androidx.appcompat.app.AppCompatActivity
import androidx.test.espresso.idling.CountingIdlingResource


class MainActivity : AppCompatActivity() {

    private val sep = "----------------------------------------"

    private lateinit var logTextBox: EditText

    // Nullable CounterIdlingResource
    private var counterIdlingResource: CountingIdlingResource? = null

    // Visible for testing only.
    @Suppress("unused")
    public fun getIdlingResource(): CountingIdlingResource? {
        if (counterIdlingResource == null) {
            counterIdlingResource = CountingIdlingResource("SKAppIdlingResource")
        }
        return counterIdlingResource
    }

    // Increment and decrement IdlingResource
    private fun decrementIdlingResource() {
        counterIdlingResource?.decrement()
    }

    private fun incrementIdlingResource() {
        counterIdlingResource?.increment()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        logTextBox = findViewById(R.id.logTextBox)
        logTextBox.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable?) {
                logTextBox.setSelection(logTextBox.text.length)
            }

            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
        })

        val provisionButton: Button = findViewById(R.id.Button1)
        provisionButton.setOnClickListener {
            addLog(sep)
            addLog("Create/Open database.")
            createDB()
        }

        val initializeButton: Button = findViewById(R.id.Button2)
        initializeButton.setOnClickListener {
            addLog(sep)
            addLog("Set value.")
            setValue()
        }

        val onlineTXButton: Button = findViewById(R.id.Button3)
        onlineTXButton.setOnClickListener {
            addLog(sep)
            addLog("Get value.")
            getValue()
        }

        val statusButton: Button = findViewById(R.id.Button4)
        statusButton.setOnClickListener {
            addLog(sep)
            addLog("Close database.")
            closeDB()
        }
    }

    // Add log message to logTextBox.
    private fun addLog(message: String) {
        runOnUiThread { logTextBox.append("$message\n") }
    }

    // Dump byte array to log with 16 bytes per line and print ASCII characters.
    private fun dumpByteArray(name: String, data: ByteArray, offset: Int = 0, length: Int = data.size) {
        val sb = StringBuilder()
        sb.append("$name: ")
        for (i in offset until offset + length) {
            if (i % 16 == 0) {
                sb.append("\n")
                sb.append(String.format("%04x: ", i))
            }
            sb.append(String.format("%02x ", data[i]))
        }
        sb.append("\n")
        for (i in offset until offset + length) {
            if (i % 16 == 0) {
                sb.append("\n")
                sb.append(String.format("%04x: ", i))
            }
            val c = data[i].toInt().toChar()
            if (c in ' '..'~') {
                sb.append(c)
            } else {
                sb.append('.')
            }
        }
        addLog(sb.toString())
    }

    // Create db
    private fun createDB() {

        Thread(Runnable {
            incrementIdlingResource()
            try {
                creteDBInternal()
            } catch (e: Exception) {
                addLog("Error: $e")
            }
            decrementIdlingResource()
        }).start()
    }

    // Create db internal
    private fun creteDBInternal() {

        // Get local data path
        val localDataPath = applicationContext.filesDir.absolutePath

        // Get external data path
        val externalDataPath = applicationContext.getExternalFilesDir(null)?.absolutePath

        // If external data path is null, use local data path
        val dataPath = externalDataPath ?: localDataPath

        addLog("Local data path: $localDataPath")
        addLog("External data path: $dataPath")

        // Create database
        sstCreate(localDataPath, dataPath)
    }

    // Set value
    private fun setValue() {

        Thread(Runnable {
            incrementIdlingResource()
            try {
                setValueInternal()
            } catch (e: Exception) {
                addLog("Error: $e")
            }
            decrementIdlingResource()
        }).start()
    }

    // Set value internal
    private fun setValueInternal() {

        // Set sample key and value
        val key = "greetings"
        val value = "Hello, World!"
        sstSet(key, value)
    }

    // Get value
    private fun getValue() {

        Thread(Runnable {
            incrementIdlingResource()
            try {
                getValueInternal()
            } catch (e: Exception) {
                addLog("Error: $e")
            }
            decrementIdlingResource()
        }).start()
    }

    // Get value internal
    private fun getValueInternal() {

        // Get sample key
        val key = "greetings"
        val value = sstGet(key)
        addLog("Value: $value")
    }

    // Close db
    private fun closeDB() {

        Thread(Runnable {
            incrementIdlingResource()
            try {
                closeDBInternal()
            } catch (e: Exception) {
                addLog("Error: $e")
            }
            decrementIdlingResource()
        }).start()
    }

    // Close db internal
    private fun closeDBInternal() {
        sstClose()
    }

    // Create or open a database
    private external fun sstCreate(localDataPath: String, externalDataPath: String): Unit

    // Set a value in the database
    private external fun sstSet(key: String, value: String): Unit

    // Get a value from the database
    private external fun sstGet(key: String): String

    // Close the database
    private external fun sstClose(): Unit

    // Load the native library on application startup
    companion object {
        init {
            System.loadLibrary("sst_native")
        }
    }
}
