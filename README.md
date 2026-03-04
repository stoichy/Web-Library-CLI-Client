CLI Movie Library Client (C / REST API)

This project is a custom-built Command Line Interface (CLI) HTTP client written entirely in C. It communicates with a remote REST API to manage a virtual movie library. The application handles user authentication, session management via cookies, and secure endpoint access using JSON Web Tokens (JWT).

Instead of relying on high-level HTTP libraries like libcurl, this client is built directly on top of POSIX Sockets (TCP), demonstrating a deep understanding of the HTTP/1.1 protocol, request formatting, and response parsing.

-- Dual-Role Authentication: Supports distinct flows for Admin (user management) and User (movie and collection management).
-- Session & Security: * Parses and stores HTTP Cookies for session persistence.
-- Handles JWT (JSON Web Tokens) for authorizing access to protected library routes (Bearer token authentication).
-- RESTful Operations: Implements GET, POST, PUT, and DELETE requests to interact with the server's API.
-- JSON Processing: Uses the lightweight parson library to serialize requests and parse API responses.
-- Robust Input Validation: Sanitizes user input locally (e.g., checking for empty strings, validating numeric ranges for years and ratings) before sending payloads to the server, reducing unnecessary network overhead.

Implementation Details
1. Network Communication & HTTP Protocol
The core of the application relies on TCP sockets to establish a reliable connection with the remote server. For every user command, the client opens a new socket, dynamically constructs the appropriate HTTP request (Headers, Body, Cookies, Authorization), and awaits the server's response.

-- Custom utility functions (requests.c, helpers.c) were developed to:
-- Format GET, POST, PUT, and DELETE HTTP strings.
-- Extract specific headers (like Set-Cookie) from the raw HTTP response.
-- Isolate the JSON payload from the HTTP body using custom parsers (json_body, server_error).

2. State & Session Management
State is managed locally in the C client:

-- Cookies: Extracted upon a successful login (admin_cookie, user_cookie) and attached to subsequent requests.
-- JWT: Upon requesting library access, the server returns a JWT. The client extracts and stores this token, appending it as an -----
-- Authorization: Bearer <token> header for all movie-related operations.
-- Logout: Locally invalidates the session by freeing the memory and setting the cookie/token pointers to NULL.

3. Data Serialization (JSON)
To interact with the REST API, data must be exchanged in application/json format.

-- Payload Creation: Functions like json_value_init_object() and json_object_set_string() are used to dynamically build the JSON strings sent in the POST and PUT request bodies.
-- Response Parsing: The client utilizes json_parse_string() to navigate the server's responses, extracting relevant information (such as generated IDs, movie arrays, or specific error messages) and presenting it in a human-readable format in the terminal.

How to Build and Run
Prerequisites: GCC compiler

-- Compilation: Simply run the make command in the root directory
-- Usage
 - Start the client interface: ./client
 - Once the prompt appears, you can use the available commands, such as:
    login_admin / login
    add_user / delete_user
    get_access
    add_movie / get_movies / update_movie / delete_movie
    add_collection / get_collections
    logout / exit
