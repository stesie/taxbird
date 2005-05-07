; this is just a stub to test gnupg signature checking ...
(display "\ntesting Taxbird's .sig-check mechanism ...\n")

(let ((sig-result (tb:check-sig "signatures/sigtest.sig")))
  (if (list? sig-result)
      (format #t "got vendor-id: ~A\nsignature-id: ~A\n"
	      (car sig-result) (cadr sig-result))

      (display "there's something wrong, see messages above.\n")))

(newline)
