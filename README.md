# Structura de date Editor
Pentru implementare este folosit un piece table. Acesta e format dintr-un ArrayList pentru textul adăugat
un string cu textul original și un arbore AVL de piese pentru eficientizarea operațiilor.

## ArrayList
* Numărul de caractere
* Capacitatea maximă
* Șirul de caractere
* Operație de inserare

Textul original este salvat ca arraylist pentru a păstra textul propriu-zis și lungimea acestuia.


## Arborele AVL
* Piesa:
  * Sursa caracterelor (original sau adăugat)
  * Poziția de start din sursă
  * Numărul de caractere la care face referință
* Numărul de caractere din subarborele stâng (este necesar pentru navigarea eficientă a arborelui)
* Adâncimea nodului (necesară pentru echilibrare)
* Operație de inserare
* Operație de ștergere
* Operație de balansare

## Operații și Complexități
### Inserare:
Se inserează la finalul ArrayList-ului textul.

1. Se găsește nodul din arbore care referențiază textul din zona în care se face inserarea.
2. Se crează un nod nou ce referențiază textul nou adăugat și înlocuiește nodul inițial.
3. Nodul inițial se împarte în două, unul care referențiază stânga zonei noi și unul dreapta.
   Dacă împărțirea se face la capătul unei secțiuni de text, unul din noduri va avea lungime 0.
   În acest caz nodul nu se crează.
4. Acestea sunt reinserate ca frunze în extremele subarborelui stâng, respectiv drept.

Complexitate: O(log(n)), unde n e nr de noduri.

### Ștergere:
Se prelucrează fiecare nod ce conține o secțiune ce trebuie eleiminată:
* Dacă tot textul din nod trebuie eliminat, nodul este șters.
* Dacă rămâne o secține continuă din nod (adică nu rămâne ceva și în dreapta și în stânga), se înlocuiește cu un un nod cu referința actualizată.
* Dacă rămâne și o secțiune dreaptă și una stângă, se înlocuiește cu nodul secțunii drepte, iar cel stâng este inserat în extrema subarborelui stâng.

Comlplexitate: O(n log(n)) (se șterge tot arborele).

Dacă considerăm o serie de inserări și ștergeri succesive, obținem un cost amortizat de O(log(n)), sugerând că implementarea este eficientă.
