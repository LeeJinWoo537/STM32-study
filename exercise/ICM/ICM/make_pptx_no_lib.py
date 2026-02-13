# -*- coding: utf-8 -*-
"""python-pptx 없이 표준 라이브러리만으로 실제 .pptx 파일 생성 (ZIP + Open XML)"""
import zipfile
import os
from io import BytesIO

OUT = r"D:\git_file\STM32\STM32-study\exercise\ICM\ICM\ICM20948_학습_발표.pptx"

# 네임스페이스
NS = {
    "p": "http://schemas.openxmlformats.org/presentationml/2006/main",
    "a": "http://schemas.openxmlformats.org/drawingml/2006/main",
    "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships",
}
P = "{%s}" % NS["p"]
A = "{%s}" % NS["a"]
R = "{%s}" % NS["r"]

def ns_tag(ns, name):
    return "{%s}%s" % (NS[ns], name)

def text_run(text):
    return (
        "<a:r><a:rPr lang=\"ko-KR\" dirty=\"0\"/><a:t xml:space=\"preserve\">%s</a:t></a:r>"
        % _escape(text)
    )

def _escape(s):
    if not s:
        return ""
    return (
        str(s)
        .replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )

def paragraph(text, font_size_pt=18):
    return (
        "<a:p><a:pPr lvl=\"0\"/><a:r><a:rPr sz=\"%d\" lang=\"ko-KR\" dirty=\"0\"/>"
        "<a:t xml:space=\"preserve\">%s</a:t></a:r></a:p>"
        % (font_size_pt * 100, _escape(text))
    )

def slide_xml(title, bullets=None):
    """한 장 슬라이드 XML: 제목 + (선택) 불릿 목록. PowerPoint 호환용 prstGeom 포함."""
    title_paras = "".join(
        paragraph(t, 44) for t in (title.split("\n") if title else [""])
    )
    # spPr에 xfrm + prstGeom 필수 (복구 오류 방지)
    title_sp = (
        "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Title\"/>"
        "<p:cNvSpPr txBox=\"1\"/><p:nvPr><p:ph type=\"title\"/></p:nvPr></p:nvSpPr>"
        "<p:spPr><a:xfrm><a:off x=\"914400\" y=\"457200\"/><a:ext cx=\"8237880\" cy=\"914400\"/></a:xfrm>"
        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
        "<p:txBody><a:bodyPr wrap=\"square\" rtlCol=\"0\"><a:spAutoFit/></a:bodyPr><a:lstStyle/>"
        + title_paras
        + "</p:txBody></p:sp>"
    )
    bullet_sp = ""
    if bullets:
        bullet_paras = "".join(paragraph(b, 18) for b in bullets)
        bullet_sp = (
            "<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Content\"/>"
            "<p:cNvSpPr txBox=\"1\"/><p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr>"
            "<p:spPr><a:xfrm><a:off x=\"914400\" y=\"1524000\"/><a:ext cx=\"8237880\" cy=\"4572000\"/></a:xfrm>"
            "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
            "<p:txBody><a:bodyPr wrap=\"square\" rtlCol=\"0\"><a:spAutoFit/></a:bodyPr><a:lstStyle/>"
            + bullet_paras
            + "</p:txBody></p:sp>"
        )
    # grpSpPr에 xfrm 필수 (chOff, chExt 포함)
    tree = (
        "<p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/>"
        "<p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"9144000\" cy=\"6858000\"/></a:xfrm></p:grpSpPr>"
        + title_sp
        + bullet_sp
        + "</p:spTree>"
    )
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<p:sld xmlns:p="%s" xmlns:a="%s" xmlns:r="%s">'
        "<p:cSld><p:bg><p:bgPr><a:solidFill><a:srgbClr val=\"FFFFFF\"/></a:solidFill>"
        "<a:effectLst/></p:bgPr><p:bgRef idx=\"1001\"><a:schemeClr val=\"bg1\"/></p:bgRef></p:bg>"
        "%s"
        "</p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>"
        % (NS["p"], NS["a"], NS["r"], tree)
    )

def main():
    slides_data = [
        ("STM32 ICM20948 학습 및 코드 분석", None),
        ("데이터시트 공부 · 드라이버 분석 · ICM_test1 프로젝트\nD:\\git_file\\STM32\\STM32-study\\exercise\\ICM\\ICM", None),
        ("목차", [
            "1. 프로젝트 개요 및 사용 자료",
            "2. ICM20948 데이터시트 공부 (note.md)",
            "3. 코드 분석 (code_Analysis, ICM20948_코드분석.md)",
            "4. ICM_test1 STM32 IDE 프로젝트",
            "5. 정리 및 활용",
        ]),
        ("1. 프로젝트 개요 · 사용 자료", [
            "note.md : 데이터시트 공부 필기 (Bank, DLPF, LSB, FSR, PLL, dps 등)",
            "code_Analysis (1).md : 헤더/소스 코드 분석",
            "ICM20948_코드분석.md : 주소·레지스터·함수별 정리, MPU9250 비교",
            "ICM_test1 : STM32CubeIDE 프로젝트 (STM32F411RE, I2C1, UART2)",
        ]),
        ("2. 데이터시트 공부 요약 (note.md)", [
            "Bank : 레지스터 페이지. 0x7F로 선택. Bank0/2/3 역할 정리.",
            "DLPF, LSB, FSR, PLL, dps 개념 정리.",
            "PWR_MGMT_2, AK09916 ST1/ST2, I2C 주소(0x68/0x69), 실무 팁.",
        ]),
        ("3. 코드 분석 요약", [
            "SelectBank, WriteReg, ReadRegs → ICM20948_Init 순서.",
            "ReadAccel / ReadGyro / ReadMag, ReadAll. ICM20948 vs MPU9250.",
        ]),
        ("4. ICM_test1 프로젝트", [
            "STM32F411RE, I2C1, UART2. Init 실패 시 LD2 깜빡임.",
            "ReadAll로 가속도·자이로·마그 9축 UART 출력.",
        ]),
        ("5. 정리 및 활용", [
            "데이터시트·코드 분석 문서로 실습 참고.",
            "MPU_STM32, README 있으면 동일 형식으로 추가 가능.",
        ]),
        ("감사합니다", ["질문 있으시면 편하게 말씀해 주세요."]),
    ]

    # 필수 부품들 (최소 구조로 실제 PowerPoint가 열 수 있는 형태)
    ct_parts = [
        '  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>',
        '  <Default Extension="xml" ContentType="application/xml"/>',
        '  <Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>',
    ]
    for i in range(1, len(slides_data) + 1):
        ct_parts.append('  <Override PartName="/ppt/slides/slide%d.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' % i)
    ct_parts.extend([
        '  <Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>',
        '  <Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>',
        '  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>',
    ])
    content_types = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">\n' + "\n".join(ct_parts) + "\n</Types>"

    rels_root = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
</Relationships>"""

    pres_rels = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="slideLayouts/slideLayout1.xml"/>
"""
    for i in range(1, len(slides_data) + 1):
        pres_rels += '  <Relationship Id="rId%d" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide%d.xml"/>\n' % (i + 2, i)

    pres_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"
  xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <p:sldSz cx="9144000" cy="6858000"/>
  <p:sldIdLst>
"""
    for i in range(len(slides_data)):
        pres_xml += '    <p:sldId id="%d" r:id="rId%d"/>\n' % (256 + i, i + 3)
    pres_xml += "  </p:sldIdLst>\n</p:presentation>"

    # 마스터/레이아웃: cSld 안에 spTree 포함 (복구 오류 방지)
    master_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldMaster xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <p:cSld>
    <p:bg><p:bgPr/><p:bgRef idx="1001"><a:schemeClr val="bg1"/></p:bgRef></p:bg>
    <p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
    <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="9144000" cy="6858000"/></a:xfrm></p:grpSpPr>
    </p:spTree>
  </p:cSld>
  <p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>
</p:sldMaster>"""

    layout_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldLayout xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <p:cSld>
    <p:bg><p:bgPr/><p:bgRef idx="1001"><a:schemeClr val="bg1"/></p:bgRef></p:bg>
    <p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
    <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="9144000" cy="6858000"/></a:xfrm></p:grpSpPr>
    </p:spTree>
  </p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sldLayout>"""

    doc_core = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:creator>ICM20948</dc:creator>
  <cp:lastModifiedBy>ICM20948</cp:lastModifiedBy>
  <dcterms:created xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:created>
  <dcterms:modified xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:modified>
</cp:coreProperties>"""

    layout_rels = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>
</Relationships>"""
    master_rels = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>
"""

    with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("[Content_Types].xml", content_types)
        zf.writestr("_rels/.rels", rels_root)
        zf.writestr("docProps/core.xml", doc_core)
        zf.writestr("ppt/presentation.xml", pres_xml)
        zf.writestr("ppt/_rels/presentation.xml.rels", pres_rels)
        zf.writestr("ppt/slideMasters/slideMaster1.xml", master_xml)
        zf.writestr("ppt/slideMasters/_rels/slideMaster1.xml.rels", master_rels)
        zf.writestr("ppt/slideLayouts/slideLayout1.xml", layout_xml)
        zf.writestr("ppt/slideLayouts/_rels/slideLayout1.xml.rels", layout_rels)
        for i, (title, bullets) in enumerate(slides_data, 1):
            zf.writestr("ppt/slides/slide%d.xml" % i, slide_xml(title, bullets))
            zf.writestr(
                "ppt/slides/_rels/slide%d.xml.rels" % i,
                '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n'
                '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">\n'
                '  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>\n'
                "</Relationships>",
            )

    print("저장 완료:", OUT)
    print("PowerPoint에서 열어서 확인해 보세요.")

if __name__ == "__main__":
    main()
