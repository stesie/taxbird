;; Copyright(C) 2005,2006 Stefan Siegl <stesie@brokenpipe.de>
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


(define tb:get-proto-filename
  (lambda (buf)
    (or (file-exists? (string-append (getenv "HOME") "/.taxbird/"))
	(mkdir (string-append (getenv "HOME") "/.taxbird/")))

    (or (file-exists? (string-append (getenv "HOME") "/.taxbird/protos/"))
	(mkdir (string-append (getenv "HOME") "/.taxbird/protos/")))

    (let ((filename (string-append (getenv "HOME") "/.taxbird/protos/"
				   (strftime "%Y%m%d%H%M%S" 
					     (localtime (current-time)))))
	  (stnr     (storage:retrieve buf "stnr"))
	  (mandant  (storage:retrieve buf "mandant")))

      (set! filename
	    (string-append 
	     filename "-"
	     
	     (if mandant mandant
		 (if stnr (string-replace
			   (string-replace stnr #\/ "") #\  "")

		     "noname"))

	     ".html"))

      filename)))
      

(define tb:get-softpse-filename
  (lambda ()
    (if (and (string? tb:config-softpse-filename)
	     (> (string-length tb:config-softpse-filename) 0))

	tb:config-softpse-filename

	;; no idea ...
	#f)))


(define tb:set-softpse-filename
  (lambda (psefn)
    (set! tb:config-softpse-filename psefn)
    (tb:config-dump)))


;;;
;;; config file stuff ...
;;;

(define tb:config-filename (string-append (getenv "HOME") "/.taxbird/config"))

;; default configuration
(define tb:config-softpse-filename "")



;; make sure ~/.taxbird/ exists ...
(let ((dir (string-append (getenv "HOME") "/.taxbird")))
  (or (file-exists? dir)
      (mkdir dir)))

;; source config file on startup ...
(and (file-exists? tb:config-filename)
     (load tb:config-filename))



(define tb:config-dump
  (lambda ()
    (let ((handle #f))
      (set! handle (open tb:config-filename (logior O_WRONLY O_CREAT)))
      (truncate-file handle 0)
      (format handle ";; taxbird configuration file~%")
      (format handle "(define tb:config-softpse-filename ~S)~%"
	      tb:config-softpse-filename)
      (close handle))))

