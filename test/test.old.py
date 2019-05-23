def forkapp(argv):
  #struct winsize wsz;
  #struct termios tios;

  #tcgetattr(STDIN_FILENO, &tios);
  #ioctl(STDIN_FILENO, TIOCGWINSZ, &wsz);

    pid, fd = os.forkpty()
    print(fd)
    print(pid)

    if (pid == 0):
        print('in exc')
        os.execvp(argv[0], argv)

  #if (*pid < 0) {
  #  error_set_errno(errno);
  #  return 0;
  #}
  #else if (*pid == 0) {
  #  execvp(argv[0], &argv[0]);
  #  error_set_errno(errno);
  #  return 0;
  #}

    return fd

    def test(self):
        fd = forkapp([self.binary] + list(self.args))
        print('is fd' , fd)
        fh = os.fdopen(fd, "w+")
        time.sleep(2)
        fh.write('W')
        fh.write('q')
        output = fh.read()

        print(output)

        #proc = subprocess.Popen([self.binary] + list(self.args), stdout=subprocess.PIPE, stdin=subprocess.PIPE)
        #proc.stdin.write('q')
        #out, err = proc.communicate(self.input)
        #print(out)
        #print(err)
        #output = out.read()
