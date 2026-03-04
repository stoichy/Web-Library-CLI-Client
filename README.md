Implementare:

1) Comunicarea cu serverul se realizeaza prin socketi TCP. Pentru fiecare comanda, se stabileste o noua conexiune cu serverul, se trimite cererea HTTP corespunzatoare
si se primeste raspunsul. Mesajele HTTP (atat cererile, cat si parsarea initiala a raspunsurilor) sunt gestionate de functiile din "requests.c" si "helpers.c". 
Implementarea functiilor pentru cererile GET si POST a pornit din scheletul laboratorului 9, cu mici adaptari la cerintele temei, iar functiile pentru DELETE si
PUT sunt in mare parte la fel ca celelalte, cu foarte mici diferente.
Implementarea primelor 4 functii este luata tot din laboratorul 9, iar functiile "extract_cookie", "json_body", "server_error" si "valid_info" sunt menite sa ajute la
afisarea erorilor, extragerea dintr-un obiect JSON si verificarea validitatii input-ului de la client.

Sesiunile de admin si utilizator sunt mentinute prin cookie-uri ("admin_cookie", "user_cookie"), fiind extrase din antetul raspunsurilor de login.
Accesul la libraria de filme este determinat de token-ul JWT (jwt_token), obtinut prin comanda "get_access". Acest token este trimis tot in antet
pentru rutele librariei.
La comenzile de "logout" si "logout_admin", cookie-urile si token-urile relevante sunt setate la "NULL" pentru a invalida sesiunea local.

2) Pentru interactiunea cu datele in format JSON (payload-uri pentru POST/PUT si corpurile raspunsurilor), am utilizat biblioteca din enunt (https://github.com/kgabis/parson).

- Functii precum "json_value_init_object()", "json_object_set_string()", "json_object_set_number()", "json_serialize_to_string()" au fost folosite pentru a crea
string-urile JSON trimise in corpul cererilor.

- Functii precum "json_parse_string()", "json_value_get_type()", "json_value_get_object()", "json_value_get_array()", "json_object_get_string()", "json_object_get_number()"
"json_array_get_count()", "json_array_get_object()" au fost utilizate pentru a naviga si extrage informatiile relevante (ID-uri, titluri, token-uri, mesaje de eroare etc.) din
corpurile JSON preluate din raspunsurile HTTP primite de la server.

3) Pe partea de client, am implementat validari pentru input-ul utilizatorului inainte de a trimite cererile la server, conform cerintelor. De exemplu, pentru comanda "add_movie",
se verifica daca titlul sau anul nu sunt goale sau scrise cu caractere invalide, iar pentru rating se verifica daca este un numar intre 0.0 si 10.0
daca anul este un numar intreg in intervalul specificat si daca rating-ul este un numar valid intre 0.0 si 10.0. Daca datele sunt invalide, se afiseaza un mesaj de eroare, iar
cererea nu este trimisa.

Sunt verificate codurile de status HTTP din raspunsurile serverului.
Pentru raspunsurile de eroare de la server, se extrage corpul JSON si se afiseaza mesajul de eroare relevant folosind functia "server_error" (care cauta campul "error"
si afiseaza valoarea acestuia).