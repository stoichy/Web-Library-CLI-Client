1) Server Communication & HTTP Requests
Communication with the server is achieved via TCP sockets. For each command, a new connection to the server is established, the corresponding HTTP request is sent, and the response is received. HTTP messages (both requests and the initial parsing of responses) are handled by the functions in requests.c and helpers.c.

The implementation of the functions for GET and POST requests started from the Lab 9 skeleton, with minor adaptations to fit the assignment requirements. The functions for DELETE and PUT are largely similar, with very slight differences.

The implementation of the first 4 helper functions is also taken from Lab 9, while the extract_cookie, json_body, server_error, and valid_info functions are designed to assist with displaying errors, extracting data from a JSON object, and verifying the validity of client input.

Admin and user sessions are maintained via cookies (admin_cookie, user_cookie), which are extracted from the header of the login responses. Access to the movie library is determined by the JWT token (jwt_token), obtained through the get_access command. This token is also sent in the header for library routes.

On logout and logout_admin commands, the relevant cookies and tokens are set to NULL to invalidate the session locally.

2) JSON Parsing
For interacting with data in JSON format (payloads for POST/PUT and response bodies), I used the library specified in the assignment (parson).

Functions such as json_value_init_object(), json_object_set_string(), json_object_set_number(), and json_serialize_to_string() were used to create the JSON strings sent in the request bodies.

Functions such as json_parse_string(), json_value_get_type(), json_value_get_object(), json_value_get_array(), json_object_get_string(), json_object_get_number(), json_array_get_count(), and json_array_get_object() were used to navigate and extract relevant information (IDs, titles, tokens, error messages, etc.) from the JSON bodies retrieved from the HTTP responses received from the server.

3) Input Validation & Error Handling
On the client side, I implemented validations for user input before sending the requests to the server, according to the requirements. For example, for the add_movie command, the program checks if the title or year are not empty or written with invalid characters; it also verifies if the year is a valid integer and if the rating is a valid number between 0.0 and 10.0. If the data is invalid, an error message is displayed, and the request is not sent.

The HTTP status codes in the server responses are checked. For error responses from the server, the JSON body is extracted, and the relevant error message is displayed using the server_error function (which searches for the error field and prints its value).