;; Copyright(C) 2006 Stefan Siegl <stesie@brokenpipe.de>
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


;;
;; first let's source the files from the autoload/ directories ...
;;

(let ((files (list)))
  (for-each
   (lambda (dirname)
     (set! dirname (string-append dirname "/autoload"))

     (if (file-exists? dirname)
	 (let ((port (opendir dirname))
	       (fn   nil))

	   (while (not (eof-object? (let ()
				      (set! fn (readdir port)) fn)))

		  (if (and (> (string-length fn) 4)
			   (string=? (substring fn (- (string-length fn) 4))
				     ".scm"))
		      
		      (if (not (member fn files))
			  (set! files (append files (list fn)))))))))
   
   (append tb:scm-directories tb:scm-directories))

  ;; okay, files contains a list of all the files to load now.
  ;; let's call back to taxbird to load them in order ...
  (for-each
   (lambda (filename)
     (tb:eval-file (string-append "autoload/" filename)))

   (sort files string<?)))

