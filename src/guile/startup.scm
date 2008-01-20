;; Copyright(C) 2006,2007 Stefan Siegl <stesie@brokenpipe.de>
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


;;
;; first let's source the files from the autoload/ directories ...
;;

(let ((files (list)))
  (for-each
   (lambda (dirname)
     (set! dirname (string-append dirname "/autoload"))

     (if (file-exists? dirname)
	 (let ((port (opendir dirname))
	       (fn   '()))

	   (while (not (eof-object? (let ()
				      (set! fn (readdir port)) fn)))

		  (if (and (> (string-length fn) 4)
			   (string=? (substring fn (- (string-length fn) 4))
				     ".scm"))
		      
		      (if (not (member fn files))
			  (set! files (append files (list fn))))))

	   (closedir port))))
   
   (append tb:scm-directories tb:scm-directories))

  ;; okay, files contains a list of all the files to load now.
  ;; let's call back to taxbird to load them in order ...
  (for-each
   (lambda (filename)
     (tb:eval-file (string-append "autoload/" filename)))

   (sort files string<?)))



;;
;; make sure we have got all the prerequisites for printing,
;; if not, warn the user one time
;;
;; (for the moment let's do so at taxbird startup, Fridtjof convincend
;; me to do so)
;;
(define tb:check-print-env
  (lambda ()
    (let ((have-lpr #f) (have-html2ps #f))
      (for-each
       (lambda (path)
	 (if (file-exists? (string-append path "/lpr"))
	     (set! have-lpr #t))

	 (if (file-exists? (string-append path "/lp"))
	     (set! have-lpr #t))

	 (if (file-exists? (string-append path "/html2ps"))
	     (set! have-html2ps #t)))

       (string-split (getenv "PATH") #\:))

      (if (not (and have-lpr have-html2ps))

	  (tb:dlg-info
	   (string-append
		    (if (not (or have-lpr have-html2ps))
			(string-append
			 "You neither have 'lpr' nor 'html2ps' installed "
			 "on your system. The former is an absolute must if "
			 "you would like to be able to print transmission "
			 "protocols from within Taxbird. 'html2ps' is "
			 "recommended for good looking results.")
			
			(if (not have-lpr)
			    (string-append
			     "You don't have 'lpr' installed on your system. "
			     "This program is absolutely necessary however, "
			     "if you would like "
			     "to be able to print transmission protocols from "
			     "within Taxbird. Please install it, if you "
			     "would like to be able to print those.")

			    (string-append
			     "You don't have 'html2ps' installed on your "
			     "system. The former is recommended to "
			     "achieve good looking transmission protocols "
			     "however.\n\n"
			     "If you don't want to print those protocols, "
			     "just ignore this. :-)")))

	   "\n\n"
	   "This message will be displayed only once."))))))

(let ((fn (string-append (getenv "HOME") "/.taxbird/no-check-print-env")))
  (or (file-exists? fn)
      (let ((handle #f))
	(tb:check-print-env)
	
	(let ((dir (string-append (getenv "HOME") "/.taxbird")))
	  (or (file-exists? dir)
	      (mkdir dir)))

	(set! handle (open fn (logior O_WRONLY O_CREAT)))
	(truncate-file handle 0)
	(format handle "Just delete this, if you want a check next time.")
	(close handle))))

