;;               -*- mode: Scheme; coding: utf-8 -*-
;;
;; Copyright(C) 2005 Stefan Siegl <stesie@brokenpipe.de>
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


(define mandverw:data '())

;; try to load defaults file ...
(let ((fn (string-append (getenv "HOME") "/.taxbird/mandverw.dat")))
  (and (file-exists? fn)
       (or (load fn) #t)))



(define mandverw:demux
  (lambda (buffer field value)
    (if (string=? field "liste")
	(mandverw:choose buffer value))

    (or (if (not (string=? field "mandverw")) #f
	    (mandverw:display-chooser buffer))

	(if (not (string=? field "store-mand")) #f
	    (mandverw:store-defaults buffer))

	(if (not (string=? field "stnr-help")) #f
	    (steuernummer:help-dialog buffer))

	(if (not (string=? field "new")) #f
	    (let ()
	      (tb:activate-sheet "mandverw-stammdaten.ui" "stammdaten")
	      #t))

	(if (not (string=? field "delete")) #f
	    (mandverw:delete-item (storage:retrieve buffer "liste")))

	(if (not (string=? field "mv:store")) #f
	    (mandverw:store buffer))

	#f)))


(define mandverw:choose
  (lambda (buffer value)
    (let ((stored (member value mandverw:data)))
      (for-each (lambda (pair)
		  (storage:store buffer (car pair) (cdr pair)))
		(cadr stored)))

    #f))


(define mandverw:display-chooser
  (lambda (buffer)
    (if buffer (storage:store buffer "liste" ""))
    (tb:activate-sheet "mandverw-chooser.ui" "chooser")

    (let ((ptr mandverw:data))
      (while (> (length ptr) 0)
	     (tb:chooser-additem "liste" (car ptr))
	     (set! ptr (cddr ptr))))

    #t))

(define mandverw:store
  (lambda (buffer)
    (if (not (and (storage:retrieve buffer "land")
		  (storage:retrieve buffer "stnr")
		  (storage:retrieve buffer "mandant")))

	(tb:dlg-error 
	 (string-append "Die Felder 'Bundesland', 'Steuernummer' sowie "
			"'Mandantenname' müssen gefüllt sein, bevor der neue "
			"Eintrag angelegt werden kann."))

	(let ((data 
	       (list (cons "land"    (storage:retrieve buffer "land"))
		     (cons "stnr"    (storage:retrieve buffer "stnr"))
		     (cons "mandant" (storage:retrieve buffer "mandant"))))
	      (stored 
	       (member (storage:retrieve buffer "mandant") mandverw:data)))

	  (if stored
	      (set-car! (cdr stored) data)
	      (set! mandverw:data 
		    (append (list (storage:retrieve buffer "mandant") data)
			    mandverw:data)))

	  (mandverw:store-file)
	  (mandverw:display-chooser buffer)))))

	



(define mandverw:store-file
  (lambda ()
    (let ((fn (string-append (getenv "HOME") "/.taxbird/"))
	  (handle #f))

      (or
       (file-exists? fn)
       (mkdir fn)) ;; create directory ~/.taxbird/
      
      ;; open file ~/.taxbird/datenlieferant.dat ...
      (set! fn (string-append fn "mandverw.dat"))
      (set! handle (open fn (logior O_WRONLY O_CREAT)))
      (truncate-file handle 0)

      ;; store defaults to the file ...
      (format handle "(define mandverw:data '~S)" mandverw:data)
      (close handle))))


(define mandverw:return-defaults
  (lambda ()
    (if (= (length mandverw:data) 2)
	(cadr mandverw:data)
	(list))))


(define mandverw:store-defaults
  (lambda (buffer)
    (if (or (> (length mandverw:data) 2)
	    (and (= (length mandverw:data) 2)
		 (not (string=? (car mandverw:data) "<Standard>"))))
	
	(tb:dlg-error (string-append "Es wurden Mandanten in der Mandanten"
				     "verwaltung angelegt. Um die Funktion "
				     "'Als Standard' nutzen zu können, müssen "
				     "diese zuerst gelöscht werden."))

	(if (and (storage:retrieve buffer "land")
		 (storage:retrieve buffer "stnr"))
	    (let ()
	      (set! mandverw:data 
		    (list "<Standard>"
			  (list 
			   (cons "land" (storage:retrieve buffer "land"))
			   (cons "stnr" (storage:retrieve buffer "stnr")))))
	      
	      (mandverw:store-file)
	      (tb:dlg-info (string-append "Die erfassten Stammdaten wurden "
					  "als Standardwerte gespeichert. "
					  "Zukünftig angelegte Steuer"
					  "erklärungen werden "
					  "automatisch mit diesen befüllt.")))

	    (tb:dlg-error (string-append "Die Pflichtfelder 'Bundesland' und "
					 "'Steuernummer' sind nicht "
					 "ausgefüllt. Bitte befüllen Sie "
					 "diese und lösen Sie die Funktion "
					 "daraufhin noch einmal aus."))))))
	



	
(define mandverw:delete-item
  (lambda (item)
    (let ((newv (list)))
      (while (> (length mandverw:data) 0)
	     (if (not (string=? (car mandverw:data) item))
		 (set! newv (append newv (list (car mandverw:data)
					       (cadr mandverw:data)))))
	     (set! mandverw:data (cddr mandverw:data)))
		 
      (set! mandverw:data newv))

    (mandverw:display-chooser #f)
    (mandverw:store-file)
    ;;(format #t "new value: ~A~%" mandverw:data)

    #t))
