;; Copyright(C) 2005,2007 Stefan Siegl <stesie@brokenpipe.de>
;; taxbird - free program to interface with German IRO's Elster/Coala
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

;; seed pseudo random number generator
(set! *random-state* (seed->random-state (current-time)))


(define export:make-elster-xml
  (lambda (transfer-header nutzdaten-header nutzdaten)
    (list "Elster" '(("xmlns" "http://www.elster.de/2002/XMLSchema")
		     ("xmlns:elster" "http://www.elster.de/2002/XMLSchema"))
	  (append transfer-header
		  (list "DatenTeil" #f
			(list "Nutzdatenblock" #f
			      (append nutzdaten-header
				      (list "Nutzdaten" #f nutzdaten))))))))


(define export:make-transfer-header
  (lambda (store daten-art vendor-id)
    (append '("TransferHeader" (("version" "8")))
	    (list (append (list "Verfahren" #f "ElsterAnmeldung"
				"DatenArt"  #f daten-art
				"Vorgang"   #f "send-NoSig")
      
			  (if (string? vendor-id)
			      ;; prepare to send real data ...
			      (list "Testmerker"   #f (if (string=? vendor-id
								"74931")
					; set testmerker, in any case, if the
					; vendor-id is 74931 ...
							  "700000004"
							  "000000000")
				    "HerstellerID" #f vendor-id)
			      
			      ;; generate a test case ...
			      (list "Testmerker"   #f "700000004"
				    "HerstellerID" #f "74931"))

			  (export:generate-datenlieferant store)
			  
			  '("Datei" #f ("Verschluesselung"    #f "PKCS#7v1.5"
					"Kompression"         #f "GZIP"

					;; setting DatenGroesse to "1", to
					;; make output file validate against
					;; schema file, real value inserted
					;; by libgeier!!
					"DatenGroesse"        #f "1"
					"TransportSchluessel" #f #f))

			  (list "VersionClient" #f (tb:get-version)))))))

  
(define export:make-nutzdaten-header
  (lambda (store vendor-id)
    (tb:eval-file "steuernummer.scm")
    (let ((land (string->number (storage:retrieve store "land")))
	  (st-nummer #f))

      (set! st-nummer
	    (steuernummer:convert land (storage:retrieve store "stnr")))

      (list "NutzdatenHeader" '(("version" "10"))
	    (append
	     (list "NutzdatenTicket" #f (number->string (random 10000000))
		   "Empfaenger" '(("id" "F")) (substring st-nummer 0 4)
		   "Hersteller" #f (list "ProduktName"    #f "Taxbird"
					 "ProduktVersion" #f
					   (format #f "Taxbird ~S, libgeier ~S"
					    (tb:get-version)
					    (tb:get-geier-version))))

	     (export:generate-datenlieferant store))))))



(define export:make-steuerfall
  (lambda (buffer art version content)
    (list "Anmeldungssteuern" (list (list "art" art) (list "version" version))
	  (list "DatenLieferant"   #f (export:make-steuerfall-datenlief buffer)
		"Erstellungsdatum" #f (strftime "%Y%m%d"
						(localtime (current-time)))
		"Steuerfall"       #f content))))


(define export:make-steuerfall-datenlief
  (lambda (buffer)
    (list "Name"    #f (storage:retrieve buffer "name-lieferant")
	  "Strasse" #f (storage:retrieve buffer "strasse-lieferant")
	  "PLZ"     #f (storage:retrieve buffer "plz-lieferant")
	  "Ort"     #f (storage:retrieve buffer "ort-lieferant"))))
