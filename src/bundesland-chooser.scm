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

(define bundesland:chooser
  '("land" "Bundesland" (string-append
			 "Bundesland von dem die erfasste Steuernummer "
			 "vergeben wurde.")
    ("Baden-Württemberg" "Bayern" "Berlin" "Brandenburg" "Bremen" "Hamburg"
     "Hessen" "Mecklenburg-Vorpommern" "Niedersachsen" "Nordrhein-Westfalen"
     "Rheinland-Pfalz" "Saarland" "Sachsen" "Sachsen-Anhalt"
     "Schleswig-Holstein" "Thüringen")))