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

(tb:eval-file "steuernummer.scm")

(define steuernummer:tests 
  '(("2893081508152" "93815/08152")
    ("9181081508155" "181/815/08155")
    ("1121081508150" "21/815/08150")
    ("3048081508155" "048/815/08155")
    ("2475081508152" "75 815 08152")
    ("2202081508156" "02/815/08156")
    ("2613081508153" "013 815 08153")
    ("4079081508151" "079/815/08151")
    ("2324081508151" "24/815/08151")
    ("5133081508159" "133/8150/8159")
    ("2722081508154" "22/815/0815/4")
    ("1010081508182" "010/815/08182")
    ("3201012312340" "201/123/12340")
    ("3101081508154" "101/815/08154")
    ("2129081508158" "29 815 08158")
    ("4151081508156" "151/815/08156")))

(let ((land 0))
  (for-each
   (lambda (pair)
     (let ((result (car pair)) (input (cadr pair))
	   (buffer (list (cons "land" (number->string land))
			 (cons "zeitraum" "1"))))

       (if (not (steuernummer:validate input buffer))
	   (tb:dlg-error
	    (format #f "Die Steuernummer ~A ist NICHT gueltig!" input))

	   (if (not (string=? (steuernummer:convert land input) result))
	       (tb:dlg-error
		(format #f "Die St-Nr. ~A wird nicht korrekt umgewandelt: ~A"
			input (steuernummer:convert land input)))))

       (set! land (+ 1 land))))
	       
     
   steuernummer:tests))
