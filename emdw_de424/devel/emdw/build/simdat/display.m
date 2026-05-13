% loads and displays the topic dirichlet counts resulting from lda
% nFigs <= number of epochs

function display(nFigs)
  waitfor = 0.25;
  x = load("cdir000.gz");
  plot(x', 'linewidth',2);
  pause(waitfor);
  for cnt = 1:nFigs-1
    waitfor = waitfor/1.1;
    fname = strcat("cdir",num2str(cnt,"%03u"),".gz");
    x = load(fname);
    plot(x', 'linewidth',2);
    pause(waitfor);
  endfor
  grid on;