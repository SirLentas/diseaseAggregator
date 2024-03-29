Δημιουργία εκτελέσιμου:
=======================
Η δημιουργία του εκτελέσιμου γίνεται με την κληση της εντολής make η οποία δημιουργεί το diseaseAggregator ,η διαγραφή του οποίου γίνεται με την make clean.


Κλήση εφαρμογής:
================
Η εφαρμογή καλείται με την εξής εντολή
./diseaseAggregator –w numWorkers -b bufferSize -i input_dir

Παράδειγμα : $  ./diseaseAggregator -w 5 -b 10 -i ./input/

Τα μέλη της εντολής αυτής μπορούν να δωθούν με οποιαδήποτε σειρά αρκεί να βρίσκονται μετά το αντίστοιχο flag .

To input μπορεί να δημιουργηθεί με την create_infiles.sh
Με ορίσματα τα diseases_file, countries_file, input_folder_name. num_of_dirs, r_per_file με αυτη τη σειρα.

Λειτουργία:
===========
Ο diseaseAggregator ειναι αυτος που αναλογα με το numWorkers δημιουργει διεργασιες με fork() και εκτελει το worker με την χρήση της execvp().
O worker καλειτε ως εξης : ./worker fifo_in fifo_out workload input_dir buffersize

Ο diseaseAggregator ειναι αυτος που δημιουργει τα named pipes και γραφει για καθε worker τις χωρες που πρεπει να διαβασει.
Οι workers διαβαζουν τη χωρα, ανοιγει τους φακελους και εισαγει σε μια λιστα τις ημερομηνιες/αρχεια. Στη συνεχεια ανοιγει τισ ημερμηνιες, διαβαζει τις
εγγραφες που ισαγει σε μια λιστα, εξαγει καταλληλα μυνηματα στο error_report.xxx (xxx το pid του) για σωστη εισοδο μιας εγγραφης ή καποιου λαθος σε εγγραφες exit
και δημιουργει τα summaries για το πατερα. Τα summaries εχουν μορφη [ Ημερομηνια / Χώρα / Ασθενεια / κρουσματα 0-20 / 21-40 / 41-60/ 60+ ]
Αφου δημιουργησει τα summaries για ολες τις χωρες που του εχουν ανατεθει ενημερωνει τον diseaseAggregator για τον αριθμο summary που θα του επιστρεψει. 
Μετα απο την αναγνωση των summaries απο τον diseaseAggregator ο τελευταιος τα τοποθετει σε μια λιστα και ζηταει  απο το χρηστη να δωσει την εντολη του.

Το προγραμμα τερματιζει μεσω της exit ή μεσω ενος σηματος SIGINT ή SIGQUIT. Στη περιπτωση του σηματος ζηταει απο το χρηστη να δωσει αν θελει μια τελευταια εντολη.
Κατα τον τερματισμο με οποιονδηποτε τροπο δημιουργει και τα logfile.xxx .

Πρωτοκολλο επικοινωνιας:
========================
Δημιουργεια δομων
-----------------
  1/ O diseaseAggregator γραφει εναν int για τον αριθμο chunks που αποτελουν ενα μυνημα/χωρα.
  2/ O worker εναν int για τον αριθμο chunks που αποτελουν ενα μυνημα/χωρα και γραφει εναν int ωστε να συνεχισει ο diseaseAggregator.
  3/ O diseaseAggregator διαβαζει το οκ και γραφει ενα ενα τα chunk, για καθε chunk περιμενει απο τον worker το οκ.
  4/ Ο worker επεξεργαζεται τους φακελους και στελνει εναν int στο diseaseAggregator για τον αριθμο των summaries που θα επιστρεψει.
  5/ Ο diseaseAggregator στελνει το οκ και περιμενει για καθε summary τον αριθμο των chunks που το αποτελουν.
  6/ Ο worker γραφει τον αριθμο των chunks, διαβαζει το οκ και γραφει ενα ενα τα chunk, για καθε chunk περιμενει απο τον diseaseAggregator το οκ.

Εντολες χρηστη. ανανεωση αρχείων και τερματισμος
------------------------------
  O worker περιμενει να διαβασει εναν int, 0 αν πρεπει να τερματισει, 1 αν πρεπει να λαβει καποια εντολη ή 2 αν εχει ληφθει καποιο μηνυμα για ανανεωση.

  [Περιπτωση 0]   Ο diseaseAggregator γραφει το 0 και στη συνεχεια περιμενει το οκ, περιμενει τους worker να τερματισουν πριν τερματισει και ο ιδιος. 
                  Αυτο συμβαινει και στην περιπτωση των SIGINT και SIGQUIT.

  [Περιπτωση 1]   Ο diseaseAggregator γραφει το 1 και στη συνεχεια περιμενει το οκ για να στειλει τον αριθμο των chunks που αποτελουν μια εντολη, διαβαζει 
                  το οκ και γραφει ενα ενα τα chunk, για καθε chunk περιμενει απο τον diseaseAggregator το οκ.

  [Περιπτωση 2]   Ο diseaseAggregator γραφει το 2 και στη συνεχεια περιμενει την απαντηση του worker.

                  > Αν απαντησει με 0 τοτε σημαινει οτι αυτος ο worker δεν εχει λαβει καποιο signal και συνεχιζει με τους επομενους worker.

                  > Αν απαντησει με το 1 τοτε σημαινει οτι εχει δεχτει καποιο signal, περιμενει ενα μηνυμα ok απο το parent και epeita στελνει εναν int στο 
                  diseaseAggregator για τον αριθμο των summaries που θα επιστρεψει. Ο diseaseAggregator στελνει το οκ και περιμενει για καθε summary τον αριθμο 
                  των chunks που το αποτελουν. Ο worker γραφει τον αριθμο των chunks, διαβαζει το οκ και γραφει ενα ενα τα chunk, για καθε chunk περιμενει 
                  απο τον diseaseAggregator το οκ.


Πολιτικη διεκπερεωσης των ερωτηματων:
=====================================

  > /listCountries
      H εφαρμογή θα τυπώνει κάθε χώρα μαζί με το process ID του Worker process που διαχειρίζεται τα αρχεία της.

  > /diseaseFrequency virusName date1 date2 [country]
      Αν δωθει country ψαχνουμε στα summaries για κρουσματα στη συγκεκριμενη χωρα, αλλιως ψαχνουμε για καθε χωρα με τη σειρα.

  > /topk-AgeRanges k country disease date1 date2
      Για τον εκαστωστε συνδυασμο ασθενεια/χωρα βρισκουμε βρισκουμε τα κρουσματα ανα ηλικιακη κατηγορια και εμφανιζουμε το μεγιστο k φορες.

  > /searchPatientRecord recordID
      To parent process προωθεί σε όλους τους Workers το αίτημα μέσω named pipes και περιμένει απάντηση απο ολους, αν ολοι επιστρεψουν αρνητικη 
      απαντηση εμφανιζει καταλληλο μηνυμα, αλλιως οποιος worker βρηκε την εγγραφη την εμφανισε.

  > /numPatientAdmissions disease date1 date2 [country]
      Αναζητουμε σειριακα στη summaries list και εμφανιζει ειτε για τη δοσμενη χωρα, ειτε για ολες τον αριθμο των κρουσματων που εισηχθησαν.

  > /numPatientDischarges disease date1 date2 [country]
      To parent process προωθεί σε όλους τους Workers το αίτημα μέσω named pipes. Αυτοι αναζητουν σειριακα στη records list και εμφανιζει 
      ειτε για τη δοσμενη χωρα, ειτε για ολες τον αριθμο των κρουσματων που πηραν βγηκαν απο το νοσοκομειο.

Δομες:
======
Οι δομες που εχουν υλοποιηθει για τα summaries και της χωρες στο diseaseAggregator και για τις εγγραφες και της ημερομηνιες στο worker
ειναι μονα συνδεδεμενες λιστες.

Έξοδος στο tty:
===============
Στο tty ο diseaseAggregator γραφει τα αποτελεσματα των εντολων.

Αρχεια εξοδου:
==============
Στο φακελο error_reports εμφανιζονται για καθε worker ενα αρχειο που αφορα τις εγγραφες.
Στο φακελο logfiles εμφανιζεται για καθε διεργασια ενα αρχειο με τα στατιστικα του.

Bash Script:
============
Λειτουργει ακριβως οπως ζητηθηκε στην εκφωνηση.
