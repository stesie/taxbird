; this is just a stub to test gnupg signature checking ...
(display "\ntesting Taxbird's .sig-check mechanism ...\n")

(let ((vendor-id (tb:check-sig "signatures/sigtest.sig")))
  (if (string? vendor-id)
      (if (= (string-length vendor-id) 5)
	  (format #t "seems to work, got vendor-id: ~A\n" vendor-id)
	  (format #t "damn, got invalid vendor-id back: ~A\n" vendor-id))

      (display "there's something wrong, see messages above.\n")))

(newline)
