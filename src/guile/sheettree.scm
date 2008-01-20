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

(define extract-sheet-tree
  (lambda (sheet-def)
    (let ((sheet-tree '()))
      (while (> (length sheet-def) 0)
	     
	     (if (string? (cadr sheet-def))
		 ;; leaf, ignore ...
		 (set! sheet-def '())
		 
		 (let ((sub-tree (extract-sheet-tree (cadr sheet-def))))
		   (set! sheet-tree 
			 (append sheet-tree 
				 (if (= (length sub-tree) 0)
				     (list (car sheet-def))
				     (list (car sheet-def) sub-tree))))

		   (set! sheet-def (cddr sheet-def)))))

      sheet-tree)))



(define get-sheet
  (lambda (sheet-name sheet-def)
    (let ((result #f))
      (while (and (not result) (> (length sheet-def) 0))
	     (if (string=? (car sheet-def) sheet-name)

		 ;; got it, return it ...
		 (set! result (if (list? (cadadr sheet-def))
				  #f ;; not a leaf ...
				  (cadr sheet-def)))

		 ;; recurse down ...
		 (if (list? (cadadr sheet-def))
		     (set! result (get-sheet sheet-name (cadr sheet-def)))))

	     (set! sheet-def (cddr sheet-def)))

      result)))

      
