#ifndef _REQUESTS_
#define _REQUESTS_

char *compute_get_request(const char *host, const char *url, const char *query_params,
	const char *cookie, const char *jwt_token);

char *compute_post_request(const char *host, const char *url, const char* content_type, 
	const char *body_data_json,
	const char *cookie, const char *jwt_token);

char *compute_put_request(const char *host, const char *url, const char* content_type,
		const char *body_data_json, const char *cookie, const char *jwt_token);

char *compute_delete_request(const char *host, const char *url,
	const char *cookie, const char *jwt_token);

#endif
