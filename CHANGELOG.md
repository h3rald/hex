<article>
    <h2>Changelog</h2>
    <ul>
<li><a href="#v0.2.0">v0.2.0</a></li>
<li><a href="#v0.1.0">v0.1.0</a></li>
</ul>

    <h3 id="v0.2.0">v0.2.0 &mdash; 2024-12-20</h3>

<h4>New Features</h4>
<ul>
    <li>Implemented a virtual machine with a bytecode compiler and interpreter.</li>
    <li><a href="https://hex.2c.fyi/spec#read-symbol">read</a>, <a href="https://hex.2c.fyi/spec#write-symbol">write</a>, <a href="https://hex.2c.fyi/spec#append-symbol">append</a> now support reading and writing from/to binary files as well.</li>
    <li><a href="https://hex.2c.fyi/spec#eval-symbol">!</a> can now evaluate a quotation of integers as hex bytecode.</li>
    <li>Increased maximum stack size to 256 items.</li>
    <li>Improved and consolidated error messages and debug messages.</li>
</ul>

<h4>Fixes</h4>
<ul>
    <li>Ensured that <a href="https://hex.2c.fyi/spec#dec-symbol">dec</a> is able to print negative integers in decimal format.</li>
    <li>Ensured that symbol identifiers cannot be longer than 256 characters.</li>
    <li>Ensured that all symbols are correctly added to the stack trace.</li>
</ul>

<h4>Chores</h4>
<ul>
    <li>Split the source code to different files, and now relying on an <a
            href="https://github.com/h3rald/hex/blob/master/scripts/amalgamate.sh">amalgamate.sh</a> script to
        concatenate them together before compiling</li>
</ul>
<h3 id="#v0.1.0">v0.1.0 &mdash; 2024-12-14</h3>

<p>Initial release, featuring:</p>
<ul>
    <li>A multi-platform executable for the <em>hex</em> interpreter.</li>
    <li>Integrated REPL.</li>
    <li>Integrated help and manual.</li>
    <li>Debug mode.</li>
    <li>0x40 (64) native symbols.</li>
    <li>Support for 32bit hexadecimal integers, strings, and quotations (lists).</li>
    <li>A complete <a href="https://hex.2c.fyi">web site</a> with more documentation and even an interactive playground.
    </li>
</ul>

</article>
