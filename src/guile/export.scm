;; Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
;; taxbird - free program to interface with German IRO's Elster/Coala
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
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

(define export:make-elster-xml
  (lambda (transfer-header nutzdaten-header nutzdaten)
    (list "Elster" '(("xmlns" "http://www.elster.de/2002/XMLSchema"))
	  (append transfer-header
		  (list "DatenTeil" #f
			(list "Nutzdatenblock" #f
			      (append nutzdaten-header
				      (list "Nutzdaten" #f nutzdaten))))))))


(define export:make-transfer-header
  (lambda (store daten-art testmerker)
    (append '("TransferHeader" (("version" "7")))
	    (list (append (list "Verfahren" #f "ElsterAnmeldung"
				"DatenArt"  #f daten-art
				"Vorgang"   #f "send-NoSig")
      
			  (if testmerker 
			      '("Testmerker"   #f "700000004"
				"HerstellerID" #f "74931")
			      (append "HerstellerID" #f 
				      (storage:retrieve store "vend-id")))

			  (export:generate-datenlieferant store)
			  
			  '("Datei" #f ("Verschluesselung"    #f "PKCS#7v1.5"
					"Kompression"         #f "GZIP"
					"TransportSchluessel" #f #f))

			  '("VersionClient" #f "0.1"))))))

  
		    
    
(define export:make-nutzdaten-header
  (lambda (store)
    (tb:eval-file "steuernummer.scm")
    (let ((land (string->number (storage:retrieve store "land")))
	  (st-nummer #f))

      (set! st-nummer
	    (steuernummer:convert land (storage:retrieve store "stnr")))

      (list "NutzdatenHeader" '(("version" "9"))
	    (append
	     (list "NutzdatenTicket" #f "123456" ; FIXME
		   "Empfaenger" '(("id" "F")) (substring st-nummer 0 4)
		   "Hersteller" #f '("ProduktName"    #f "Taxbird"
				     "ProduktVersion" #f "V. 0.1"))
	     (export:generate-datenlieferant store))))))



(define export:make-steuerfall
  (lambda (buffer art version content)
    (list "Anmeldungssteuern" (list (list "art" art) (list "version" version))
	  (list "DatenLieferant"   #f (export:make-steuerfall-datenlief buffer)
		"Erstellungsdatum" #f "20050212" ; FIXME
		"Steuerfall"       #f content))))


(define export:make-steuerfall-datenlief
  (lambda (buffer)
    (list "Name"    #f (storage:retrieve buffer "name")
	  "Strasse" #f (storage:retrieve buffer "strasse")
	  "PLZ"     #f (storage:retrieve buffer "plz")
	  "Ort"     #f (storage:retrieve buffer "ort"))))