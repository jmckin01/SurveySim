pro counts
  
  res = mrdfits('output.fits',6,head,/silent)
  alpha = 0.159                ;one std deviation
  plusfrac = (1.0-alpha)
  minusfrac = alpha

  chis = res.chisq
  gpts = where(chis gt median(chis))
  res = res[gpts]

  c1=res.dnds250
  c2=res.dnds350
  c3=res.dnds500

  c1mean = []
  c2mean = []
  c3mean = []
  c1plus = []
  c2plus = []
  c3plus = []
  c1minus = []
  c2minus = []
  c3minus = []

  c1size = n_elements(res[0].dnds250)
  c2size = n_elements(res[0].dnds350)
  c3size = n_elements(res[0].dnds500)  
  
  for i=0,c1size-1 do begin
     dnds = c1[i,*]
     dnds = dnds[sort(dnds)]
     pi = plusfrac*n_elements(dnds)
     mi = minusfrac*n_elements(dnds)
     c1mean = [c1mean,mean(dnds)]
     c1plus = [c1plus,dnds[pi]]
     c1minus = [c1minus,dnds[mi]]
  endfor

  for i=0,c2size-1 do begin
     dnds = c2[i,*]
     dnds = dnds[sort(dnds)]
     pi = plusfrac*n_elements(dnds)
     mi = minusfrac*n_elements(dnds)
     c2mean = [c2mean,mean(dnds)]
     c2plus = [c2plus,dnds[pi]]
     c2minus = [c2minus,dnds[mi]]
  endfor

  for i=0,c3size-1 do begin
     dnds = c3[i,*]
     dnds = dnds[sort(dnds)]
     pi = plusfrac*n_elements(dnds)
     mi = minusfrac*n_elements(dnds)
     c3mean = [c3mean,mean(dnds)]
     c3plus = [c3plus,dnds[pi]]
     c3minus = [c3minus,dnds[mi]]
  endfor

  set_plot,'x'

  x = indgen(n_elements(c1mean))
  plot,x,c1mean,xrange=[0,16],xstyle=1
  oplot,x,c1plus,linestyle=1
  oplot,x,c1minus,linestyle=1

end