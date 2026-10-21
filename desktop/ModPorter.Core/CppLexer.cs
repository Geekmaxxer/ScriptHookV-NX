namespace ModPorter.Core;

public sealed record Token(string Text, int Start, int Length, int Line);
public static class CppLexer {
    // Comments and literals are blanked in place so diagnostics/edits retain exact offsets.
    public static string CodeOnly(string text) {
        var chars = text.ToCharArray(); int i = 0;
        void Blank(int a, int b) { for (int k = a; k < b; k++) if (chars[k] != '\n' && chars[k] != '\r') chars[k] = ' '; }
        while (i < text.Length) {
            int start = i;
            if (i + 1 < text.Length && text[i] == '/' && text[i + 1] == '/') {
                while (i < text.Length && text[i] != '\n') i++; Blank(start, i);
            } else if (i + 1 < text.Length && text[i] == '/' && text[i + 1] == '*') {
                i += 2; while (i + 1 < text.Length && !(text[i] == '*' && text[i + 1] == '/')) i++; i = Math.Min(text.Length, i + 2); Blank(start, i);
            } else if (text[i] == 'R' && i + 1 < text.Length && text[i + 1] == '"') {
                int p = text.IndexOf('(', i + 2);
                if (p < 0 || p - i > 18) { i++; continue; }
                string end = ")" + text[(i + 2)..p] + "\""; int stop = text.IndexOf(end, p + 1, StringComparison.Ordinal);
                i = stop < 0 ? text.Length : stop + end.Length; Blank(start, i);
            } else if (text[i] is '"' or '\'') {
                char quote = text[i++];
                while (i < text.Length) { if (text[i] == '\\') { i = Math.Min(text.Length, i + 2); continue; } if (text[i++] == quote) break; }
                Blank(start, i);
            } else i++;
        }
        return new string(chars);
    }
    public static List<Token> Tokens(string text) {
        string code = CodeOnly(text); var result = new List<Token>(); int i = 0, line = 1;
        while (i < code.Length) {
            if (result.Count >= 500000) throw new InvalidDataException("C++ token limit exceeded; split very large source files before porting.");
            char c = code[i]; if (c == '\n') { line++; i++; continue; } if (char.IsWhiteSpace(c)) { i++; continue; }
            int start = i++;
            if (char.IsAsciiLetterOrDigit(c) || c == '_') while (i < code.Length && (char.IsAsciiLetterOrDigit(code[i]) || code[i] == '_')) i++;
            else if (i < code.Length && new[] { "::", "==", "!=", "<=", ">=", "++", "--", "&&", "||", "<<", ">>", "+=", "-=", "*=", "/=", "->" }.Contains(code[start..(i + 1)])) i++;
            result.Add(new(code[start..i], start, i - start, line));
        }
        return result;
    }
    public static int Match(IReadOnlyList<Token> t, int at, string open, string close) {
        int depth = 0;
        for (int i = at; i < t.Count; i++) { if (t[i].Text == open) depth++; else if (t[i].Text == close && --depth == 0) return i; }
        return -1;
    }
}
