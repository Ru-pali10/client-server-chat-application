# Chatbot Application

A simple chatbot application for network programming using C and sockets.

---

## Requirements

<ul>
    <li>Linux operating system (Ubuntu, Pop OS, or Linux Mint recommended)</li>
    <li>C programming language</li>
    <li>gcc compiler</li>
    <li>VirtualBox or similar software (if using a non-Linux OS)</li>
    <li>Basic knowledge of network programming</li>
</ul>

---

## Compilation

<ul>
    <li>Run the following command to compile the server:
        <pre><code>gcc -o ./socket/server ./socket/server.c ./socket/uuid4.c -lpthread</code></pre>
    </li>
</ul>

---

## Running the Application

<ul>
    <li>Start the server:
        <pre><code>./socket/server</code></pre>
    </li>
    <li>Start the client:
        <pre><code>./socket/client</code></pre>
    </li>
</ul>

---

## Chatbot Commands

<ul>
    <li>Activate chatbot: <code>/chatbot login</code></li>
    <li>Deactivate chatbot: <code>/chatbot logout</code></li>
</ul>

---

## Additional Commands

<ul>
    <li>Retrieve a list of active clients: <code>/active</code></li>
    <li>Send a message:
        <pre><code>/send &lt;dest_id&gt; &lt;message&gt;</code></pre>
    </li>
    <li>Logout from the application: <code>/logout</code></li>
</ul>

---

## Chat History Management

<ul>
    <li>Retrieve chat history with a recipient:
        <pre><code>/history &lt;recipient_id&gt;</code></pre>
    </li>
    <li>Delete chat history with a recipient:
        <pre><code>/history_delete &lt;recipient_id&gt;</code></pre>
    </li>
    <li>Delete all chat history:
        <pre><code>/delete_all</code></pre>
    </li>
</ul>

---

## Chatbot V2 Commands

<ul>
    <li>Activate enhanced chatbot: <code>/chatbot_v2 login</code></li>
    <li>Deactivate enhanced chatbot: <code>/chatbot_v2 logout</code></li>
</ul>

---

## Log File

<ul>
    <li>A log file named <code>server_log.txt</code> is automatically created to record server activities.</li>
</ul>

---

## Notes

<ul>
    <li>This project requires basic knowledge of network programming in C.</li>
    <li>Ensure the <code>gcc</code> compiler is installed on your Linux system before running the program.</li>
</ul>

---


### Contact

<ul>
    <li>For any queries, feel free to contact <a href="mailto:rupalikalundia10@gmail.com">Rupali </a>.</li>
</ul>
