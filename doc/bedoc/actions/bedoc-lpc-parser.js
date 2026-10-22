/**
 * @file LPC parser for the gLPU simul_efun help generator.
 *
 * Parses LPC source with the LPC language server's compiler
 * (`@lpc-lang/core`) and walks the resulting syntax tree, producing the
 * structured `functions` payload consumed by the help formatter.
 *
 * The compiler attaches each LPCDoc comment block to the declaration it
 * documents, so there are no heuristics for pairing blocks with functions:
 *   - only top-level function definitions (with a body) that carry a doc block
 *     are emitted; file headers and documented variables never attach to one,
 *   - `private` functions are excluded (not part of the public API),
 *   - signatures come from the syntax tree, `async` functions included,
 *   - tag types are taken verbatim from the braces, so unions with spaces,
 *     macro types, quoted object paths, variadics and the like all survive,
 *   - `@overload` parameter lists are flattened into one,
 *   - free-form `@errors` / `@throws` tags are captured.
 *
 * `#include` directives are answered with empty source: documentation
 * extraction needs no macro expansion, and the compiler requires a file
 * handler to get past them.
 *
 * @author gesslar
 */

import lpc from "@lpc-lang/core"

const ACCESS = ["public", "protected", "private"]
const MODIFIERS = ["async", "nomask", "varargs"]

const fileHandler = Object.freeze({
  loadIncludeFile: (_source, filename) => ({filename, source: ""}),
  loadInclude: (_source, filename) => ({uri: filename, source: ""}),
})

export default class LpcParser {
  static meta = Object.freeze({
    kind: "parser",
    input: "lpc",
    terms: "ref://./bedoc-lpc-parser.yaml"
  })

  setup = builder => builder
    .do("Parse source", this.#parse)

  #parse = ctx => {
    const sourceFile = lpc.createSourceFile(
      "source.lpc",
      String(ctx),
      {languageVersion: lpc.ScriptTarget.Latest, fileHandler},
      true,
      undefined,
      // FluffOS-only syntax such as `async` is not recognized otherwise.
      lpc.LanguageVariant.FluffOS
    )

    const functions = []

    lpc.forEachChild(sourceFile, node => {
      if(!lpc.isFunctionDeclaration(node) || !node.body || !node.name)
        return

      if(!node.jsDoc?.length)
        return

      const signature = this.#signature(node, sourceFile)

      if(signature.access === "private")
        return

      functions.push(this.#function(node, signature, sourceFile))
    })

    return {functions}
  }

  // -- Signature ------------------------------------------------------------

  #signature = (node, sourceFile) => {
    const keywords = (node.modifiers ?? []).map(m => m.getText(sourceFile))
    const [modifier1, modifier2] = keywords.filter(k => MODIFIERS.includes(k))

    let type = node.type?.getText(sourceFile).trim() ?? "mixed"
    const array = /\*$/.test(type)

    if(array)
      type = type.replace(/\s*\*$/, "")

    return this.#gimme({
      access: keywords.find(k => ACCESS.includes(k)),
      modifier1,
      modifier2,
      type,
      array,
      name: node.name.text,
      parameters: node.parameters.map(p => p.getText(sourceFile).trim()),
    })
  }

  // -- Doc block ------------------------------------------------------------

  // The compiler decides which block documents the function, but its JSDoc
  // type parser does not understand LPC types (`{object STD_ROOM}`,
  // `{string | STD_BODY}`) and nests `@overload` parameters out of reach. The
  // tags are therefore read from the block's raw text, matching braces, which
  // keeps types verbatim and flattens overloads into one parameter list.
  #function = (node, signature, sourceFile) => {
    // Only the block nearest the function documents it.
    const {description, tags} = this.#readBlock(node.jsDoc.at(-1).getText(sourceFile))
    const named = (...names) => tags.filter(t => names.includes(t.tag))

    const result = {name: node.name.text, description, signature}

    const params = named("param").map(t => this.#typed(t, true))
    if(params.length)
      result.param = params

    const [ret] = named("return", "returns").map(t => this.#typed(t, false))
    if(ret)
      result.return = {type: ret.type, content: ret.content}

    const errors = named("errors", "throws")
    if(errors.length)
      result.errors = errors.flatMap(t => this.#dropDash(t.lines))

    const examples = named("example", "examples")
    if(examples.length)
      result.example = examples.flatMap(t => t.lines)

    return result
  }

  // Splits a `/** ... */` block into description lines and tags, each tag
  // holding the lines from its `@name` up to the next tag. Lines keep any
  // indentation past the ` * ` margin so examples survive intact.
  #readBlock = text => {
    const lines = text
      .replace(/^\/\*\*/, "")
      .replace(/\*\/$/, "")
      .split(/\r?\n/)
      .map(line => line.replace(/^\s*\* ?/, "").trimEnd())

    while(lines.length && lines.at(-1) === "")
      lines.pop()

    const description = []
    const tags = []

    for(const line of lines) {
      const tag = line.match(/^@(\w+)\s?(.*)$/)

      if(tag)
        tags.push({tag: tag[1], lines: tag[2] ? [tag[2]] : []})
      else if(tags.length)
        tags.at(-1).lines.push(line)
      else
        description.push(line)
    }

    while(description.length && description[0].trim() === "")
      description.shift()

    return {description, tags}
  }

  // Reads `{type} [name] - text` off a tag, taking the type and a bracketed
  // `[name=default]` each up to its matching close, so nesting such as
  // `[init=arr[0]]` survives. Parameters carry a name; return values do not.
  #typed = ({lines}, hasName) => {
    let [first = "", ...rest] = lines
    let type = "mixed"

    if(first.startsWith("{")) {
      const end = this.#closing(first, "{", "}")

      type = first.slice(1, end).trim() || "mixed"
      first = first.slice(end + 1).trimStart()
    }

    let name

    if(hasName) {
      name = first.startsWith("[")
        ? first.slice(0, this.#closing(first, "[", "]") + 1)
        : first.match(/^\S*/)[0]
      first = first.slice(name.length).trimStart()
    }

    const content = this.#dropDash([first, ...rest])
      .filter((line, i) => i > 0 || line !== "")

    return hasName ? {type, name, content} : {type, content}
  }

  // Index of the `close` matching the `open` that begins `text`, or the end of
  // the text when it is unbalanced.
  #closing = (text, open, close) => {
    let depth = 0

    for(let i = 0; i < text.length; i++) {
      if(text[i] === open)
        depth++
      else if(text[i] === close && --depth === 0)
        return i
    }

    return text.length
  }

  // Tag text conventionally opens with a `- ` separator; drop it.
  #dropDash = ([first = "", ...rest]) => [first.replace(/^-\s+/, ""), ...rest]

  #gimme = ob =>
    Object.fromEntries(Object.entries(ob).filter(([_, v]) => v != null))
}
