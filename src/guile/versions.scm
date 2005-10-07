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

(define taxbird-version (string-split (tb:get-version) #\.))
(define taxbird-major (string->number (car taxbird-version)))
(define taxbird-minor (string->number (cadr taxbird-version)))

(define require-version
  (lambda (major minor sheet-name)
    (let ((ret (or (> taxbird-major major)
		   (and (= taxbird-major major)
			(>= taxbird-minor minor)))))
      (if (not ret)
	  (tb:dlg-error 
	   (format #f (string-append "The installed sheet `~A' cannot be "
				     "used with your Taxbird installation, "
				     "since it is to old. Please do a real "
				     "update of the binary application. "
				     "Until then you won't be able to use "
				     "that sheet.")
		   sheet-name)))

      ret)))
