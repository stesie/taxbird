;; Copyright(C) 2005 Stefan Siegl <stesie@brokenpipe.de>
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
      